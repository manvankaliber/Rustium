// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/raw_ref.h"
#include "net/dns/dns_transaction.h"

#include <stdint.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/base64url.h"
#include "base/containers/circular_deque.h"
#include "base/containers/span.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/memory/safe_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/numerics/byte_conversions.h"
#include "base/numerics/safe_conversions.h"
#include "base/rand_util.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/single_thread_task_runner.h"
#include "base/threading/thread_checker.h"
#include "base/timer/elapsed_timer.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "build/build_config.h"
#include "net/base/backoff_entry.h"
#include "net/base/completion_once_callback.h"
#include "net/base/elements_upload_data_stream.h"
#include "net/base/features.h"
#include "net/base/idempotency.h"
#include "net/base/io_buffer.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/base/network_handle.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/dns/dns_attempt.h"
#include "net/dns/dns_config.h"
#include "net/dns/dns_http_attempt.h"
#include "net/dns/dns_names_util.h"
#include "net/dns/dns_platform_attempt_factory.h"
#include "net/dns/dns_query.h"
#include "net/dns/dns_response.h"
#include "net/dns/dns_response_result_extractor.h"
#include "net/dns/dns_server_iterator.h"
#include "net/dns/dns_session.h"
#include "net/dns/dns_tcp_attempt.h"
#include "net/dns/dns_udp_attempt.h"
#include "net/dns/dns_udp_tracker.h"
#include "net/dns/dns_util.h"
#include "net/dns/host_cache.h"
#include "net/dns/host_resolver.h"
#include "net/dns/host_resolver_internal_result.h"
#include "net/dns/public/dns_over_https_config.h"
#include "net/dns/public/dns_over_https_server_config.h"
#include "net/dns/public/dns_protocol.h"
#include "net/dns/public/dns_query_type.h"
#include "net/dns/public/secure_dns_policy.h"
#include "net/dns/resolve_context.h"
#include "net/http/http_request_headers.h"
#include "net/log/net_log.h"
#include "net/log/net_log_capture_mode.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_source.h"
#include "net/log/net_log_values.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/datagram_client_socket.h"
#include "net/socket/stream_socket.h"
#include "net/third_party/uri_template/uri_template.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "url/url_constants.h"

#include <array>
#include <iterator>
#include "net/dns/dns_request_family_ffi.h"
namespace net {

namespace {

bool IsIPLiteral(const std::string& hostname) {
  IPAddress ip;
  return ip.AssignFromIPLiteral(hostname);
}

base::DictValue NetLogStartParams(const std::string& hostname, uint16_t qtype) {
  base::DictValue dict;
  dict.Set("hostname", hostname);
  dict.Set("query_type", qtype);
  return dict;
}

void AppendDnsHTTPAttempt(base::WeakPtr<ResolveContext> context,
                          DnsSession* session, size_t index,
                          std::unique_ptr<DnsQuery> query,
                          std::vector<std::unique_ptr<DnsAttempt>>* attempts,
                          URLRequestContext* url_context,
                          RequestPriority priority, bool probe) {
  DCHECK(url_context);
  DCHECK_LT(index, session->config().doh_config.servers().size());
  const auto& server = session->config().doh_config.servers().at(index);
  GURL url(GetURLFromTemplateWithoutParameters(server.server_template()));
  attempts->push_back(std::make_unique<DnsHTTPAttempt>(
      std::move(context), session, index, std::move(query),
      server.server_template(), url, server.use_post(), url_context, priority, probe));
}

// ----------------------------------------------------------------------------

const net::BackoffEntry::Policy kProbeBackoffPolicy = {
    // Apply exponential backoff rules after the first error.
    0,
    // Begin with a 1s delay between probes.
    1000,
    // Increase the delay between consecutive probes by a factor of 1.5.
    1.5,
    // Fuzz the delay between consecutive probes between 80%-100% of the
    // calculated time.
    0.2,
    // Cap the maximum delay between consecutive probes at 1 hour.
    1000 * 60 * 60,
    // Never expire entries.
    -1,
    // Do not apply an initial delay.
    false,
};

// Probe runner that continually sends test queries (with backoff) to DoH
// servers to determine availability.
//
// Expected to be contained in request classes owned externally to HostResolver,
// so no assumptions are made regarding cancellation compared to the DnsSession
// or ResolveContext. Instead, uses WeakPtrs to gracefully clean itself up and
// stop probing after session or context destruction.
class DnsOverHttpsProbeRunner : public DnsProbeRunner {
 public:
  DnsOverHttpsProbeRunner(base::WeakPtr<DnsSession> session,
                          base::WeakPtr<ResolveContext> context)
      : session_(session), context_(context) {
    RustFrame frame(*this); frame.Run(dns_request_family::ProbeEvent::kInitialize);
}

  ~DnsOverHttpsProbeRunner() override = default;

  void Start(bool network_change) override {
    RustFrame frame(*this); frame.Run(dns_request_family::ProbeEvent::kStart, dns_request_family::W(network_change));
}

  base::TimeDelta GetDelayUntilNextProbeForTest(
      size_t doh_server_index) const override {
    RustFrame frame(*this);
    return base::Microseconds(frame.Run(dns_request_family::ProbeEvent::kDelay, dns_request_family::W(0, doh_server_index)).x);
}

 private:
  struct ProbeStats {
    ProbeStats()
        : backoff_entry(
              std::make_unique<net::BackoffEntry>(&kProbeBackoffPolicy)) {}

    std::unique_ptr<net::BackoffEntry> backoff_entry;
    std::vector<std::unique_ptr<DnsAttempt>> probe_attempts;
    base::WeakPtrFactory<ProbeStats> weak_factory{this};
  };

  class RustFrame {
   public:
    explicit RustFrame(const DnsOverHttpsProbeRunner& runner) : c(runner) {}
    explicit RustFrame(DnsOverHttpsProbeRunner& runner) : c(runner), m(&runner) {}
    base::WeakPtr<ProbeStats> stats;
    base::TimeTicks sequence_start;
    base::TimeTicks query_start;
    DnsResolverWords Run(dns_request_family::ProbeEvent event, DnsResolverWords in = {}) {
      return rustium_dns_request_family_run(5, static_cast<uint32_t>(event), in, this, &Call);
    }
   private:
    using O = dns_request_family::ProbeOp;
    static DnsResolverWords Call(void* context, uint32_t op, DnsResolverWords in) {
      return static_cast<RustFrame*>(context)->Native(static_cast<O>(op), in);
    }
    DnsOverHttpsProbeRunner& Mutable() { CHECK(m); return *m; }
    const HostResolverInternalResult& Result(uint64_t i) const {
      return **std::next(extracted->value().begin(), i);
    }
    DnsResolverWords Native(O op, DnsResolverWords in) {
      using dns_request_family::W;
      switch (op) {
        case O::kRead:
          switch (in.x) {
            case 0: return W(!!c->session_);
            case 1: return W(!!c->context_);
            case 2: return W(0, c->session_->config().doh_config.servers().size());
            case 3: return W(0, c->probe_stats_list_.size());
            case 4: return W(!!stats);
            case 5: return W(c->context_->GetDohServerAvailability(in.a, c->session_.get()));
            case 6: return W(0, stats->probe_attempts.size());
            default: NOTREACHED();
          }
        case O::kCheck:
          switch (in.x) {
            case 0: DCHECK(c->session_); DCHECK(!c->session_->config().doh_config.servers().empty()); DCHECK(c->context_); break;
            case 1: DCHECK(c->session_); DCHECK(c->context_); break;
            case 2: DCHECK(stats); DCHECK(stats->backoff_entry); break;
            case 3: DCHECK_LT(in.a, stats->probe_attempts.size()); break;
            default: NOTREACHED();
          }
          return {};
        case O::kEncodeProbe: {
          auto encoded = dns_names_util::DottedNameToNetwork(kDohProbeHostname);
          DCHECK(encoded); Mutable().formatted_probe_qname_ = std::move(*encoded); return {};
        }
        case O::kResize: Mutable().probe_stats_list_.resize(in.a); return {};
        case O::kStatsPresent: return W(!!c->probe_stats_list_.at(in.a));
        case O::kNewStats:
          Mutable().probe_stats_list_.at(in.a) = std::make_unique<ProbeStats>();
          stats = c->probe_stats_list_.at(in.a)->weak_factory.GetWeakPtr(); return {};
        case O::kStatsClear: Mutable().probe_stats_list_.at(in.a) = nullptr; return {};
        case O::kClearAll: Mutable().probe_stats_list_.clear(); return {};
        case O::kClockNow: sequence_start = base::TimeTicks::Now(); return {};
        case O::kBackoffFailure: stats->backoff_entry->InformOfRequest(false); return {};
        case O::kPostContinue:
          base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(FROM_HERE,
              base::BindOnce(&DnsOverHttpsProbeRunner::ContinueProbe, Mutable().weak_ptr_factory_.GetWeakPtr(), in.a, stats, !!in.x, sequence_start), stats->backoff_entry->GetTimeUntilRelease()); return {};
        case O::kConstructHttp: {
          std::unique_ptr<DnsQuery> query;
          switch (in.b) {
            case 1: query = std::make_unique<DnsQuery>(0, c->formatted_probe_qname_, dns_protocol::kTypeA, nullptr, DnsQuery::PaddingStrategy::BLOCK_LENGTH_128); break;
            case 0: query = std::make_unique<DnsQuery>(*stats->probe_attempts.front()->GetQuery()); break;
            default: NOTREACHED();
          }
          AppendDnsHTTPAttempt(c->context_->GetWeakPtr(), c->session_.get(), in.a, std::move(query), &stats->probe_attempts, c->context_->url_request_context(), RequestPriority::DEFAULT_PRIORITY, true); return {};
        }
        case O::kStartAttempt:
          stats->probe_attempts.back()->Start(base::BindOnce(&DnsOverHttpsProbeRunner::ProbeComplete,
              Mutable().weak_ptr_factory_.GetWeakPtr(), static_cast<uint32_t>(in.a), in.b,
              stats, !!in.x, sequence_start, base::TimeTicks::Now())); return {};
        case O::kRecordFailure:
          c->context_->RecordServerFailure(in.b, DnsTransactionFactory::AttemptMode::kHttp, in.x, c->session_.get()); return {};
        case O::kResponsePresent:
          response = stats->probe_attempts.at(in.a)->GetResponse(); return W(!!response);
        case O::kExtractResponse: {
          DnsResponseResultExtractor extractor(*response);
          extracted = extractor.ExtractDnsResults(DnsQueryType::A, kDohProbeHostname, 0); return {};
        }
        case O::kResultCount:
          return extracted->has_value() ? W(0, extracted->value().size(), 1) : W();
        case O::kResultIsData: return W(Result(in.a).type() == HostResolverInternalResult::Type::kData);
        case O::kResultHasEndpoints: return W(!Result(in.a).AsData().endpoints().empty());
        case O::kRecordSuccess: c->context_->RecordServerSuccess(in.a, DnsTransactionFactory::AttemptMode::kHttp, c->session_.get()); return {};
        case O::kRecordRtt:
          c->context_->RecordRtt(in.a, DnsTransactionFactory::AttemptMode::kHttp, base::TimeTicks::Now() - query_start, in.x, c->session_.get()); return {};
        case O::kRecordSequence: {
          static constexpr std::array<std::string_view, 4> names{
              "Net.DNS.ProbeSequence.ConfigChange.Failure.AttemptTime", "Net.DNS.ProbeSequence.ConfigChange.Success.AttemptTime",
              "Net.DNS.ProbeSequence.NetworkChange.Failure.AttemptTime", "Net.DNS.ProbeSequence.NetworkChange.Success.AttemptTime"};
          base::UmaHistogramLongTimes(names.at(in.a), base::TimeTicks::Now() - sequence_start); return {};
        }
        case O::kDelay: return W(c->probe_stats_list_.at(in.a)->backoff_entry->GetTimeUntilRelease().InMicroseconds());
        default: NOTREACHED();
      }
    }
    raw_ref<const DnsOverHttpsProbeRunner> c;
    raw_ptr<DnsOverHttpsProbeRunner> m = nullptr;
    raw_ptr<const DnsResponse> response = nullptr;
    std::optional<DnsResponseResultExtractor::ResultsOrError> extracted;
  };

  void ContinueProbe(size_t doh_server_index,
                     base::WeakPtr<ProbeStats> probe_stats,
                     bool network_change,
                     base::TimeTicks sequence_start_time) {
    RustFrame frame(*this); frame.stats = std::move(probe_stats); frame.sequence_start = sequence_start_time;
    frame.Run(dns_request_family::ProbeEvent::kContinue, dns_request_family::W(network_change, doh_server_index));
}

  void ProbeComplete(uint32_t attempt_number,
                     size_t doh_server_index,
                     base::WeakPtr<ProbeStats> probe_stats,
                     bool network_change,
                     base::TimeTicks sequence_start_time,
                     base::TimeTicks query_start_time,
                     int rv) {
    RustFrame frame(*this); frame.stats = std::move(probe_stats);
    frame.sequence_start = sequence_start_time; frame.query_start = query_start_time;
    frame.Run(dns_request_family::ProbeEvent::kComplete, dns_request_family::W(rv, attempt_number, doh_server_index, network_change));
}

  base::WeakPtr<DnsSession> session_;
  base::WeakPtr<ResolveContext> context_;
  std::vector<uint8_t> formatted_probe_qname_;

  // List of ProbeStats, one for each DoH server, indexed by the DoH server
  // config index.
  std::vector<std::unique_ptr<ProbeStats>> probe_stats_list_;

  base::WeakPtrFactory<DnsOverHttpsProbeRunner> weak_ptr_factory_{this};
};

// ----------------------------------------------------------------------------

// Implements DnsTransaction. Configuration is supplied by DnsSession.
// The suffix list is built according to the DnsConfig from the session.
// The fallback period for each DnsUDPAttempt is given by
// ResolveContext::NextClassicFallbackPeriod(). The first server to attempt on
// each query is given by ResolveContext::NextFirstServerIndex, and the order is
// round-robin afterwards. Each server is attempted DnsConfig::attempts times.
class DnsTransactionImpl final : public DnsTransaction {
 public:
  DnsTransactionImpl(
      DnsSession* session,
      std::string hostname,
      uint16_t qtype,
      const NetLogWithSource& parent_net_log,
      // TODO(crbug.com/396483553): Remove `opt_rdata` and all plumbing between
      // DnsTransactionImpl and DnsQuery in a follow-up CL now that structured
      // DNS error (EDE) requests are injected at the DnsQuery layer.
      const OptRecordRdata* opt_rdata,
      DnsTransactionFactory::AttemptMode attempt_mode,
      SecureDnsMode secure_dns_mode,
      handles::NetworkHandle target_network,
      ResolveContext* resolve_context,
      bool fast_timeout)
      : session_(session),
        hostname_(std::move(hostname)),
        qtype_(qtype),
        opt_rdata_(opt_rdata),
        attempt_mode_(attempt_mode),
        secure_dns_mode_(secure_dns_mode),
        target_network_(target_network),
        fast_timeout_(fast_timeout),
        net_log_(NetLogWithSource::Make(NetLog::Get(),
                                        NetLogSourceType::DNS_TRANSACTION)),
        resolve_context_(resolve_context->AsSafeRef()) {
    DCHECK(session_.get());
    DCHECK(!hostname_.empty());
    DCHECK(!IsIPLiteral(hostname_));
    parent_net_log.AddEventReferencingSource(NetLogEventType::DNS_TRANSACTION,
                                             net_log_.source());
  }

  DnsTransactionImpl(const DnsTransactionImpl&) = delete;
  DnsTransactionImpl& operator=(const DnsTransactionImpl&) = delete;

  ~DnsTransactionImpl() override {
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    RustFrame frame(*this);
    frame.Run(dns_request_family::TransactionEvent::kDestroy);
}

  const std::string& GetHostname() const override {
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    return hostname_;
  }

  uint16_t GetType() const override {
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    return qtype_;
  }

  void Start(ResponseCallback callback) override {
    RustFrame frame(*this); frame.incoming_callback = std::move(callback);
    frame.Run(dns_request_family::TransactionEvent::kStart);
}

  void SetRequestPriority(RequestPriority priority) override {
    request_priority_ = priority;
  }

  std::optional<DohResolutionDetails> GetDohResolutionDetails() const override {
    return doh_details_;
  }

 private:
  // Wrapper for the result of a DnsUDPAttempt.
  struct AttemptResult {
    AttemptResult() = default;
    AttemptResult(int rv, const DnsAttempt* attempt)
        : rv(rv), attempt(attempt) {}

    int rv;
    raw_ptr<const DnsAttempt, AcrossTasksDanglingUntriaged> attempt;
  };

  // Used in UMA (DNS.AttemptType). Do not renumber or remove values.
  enum class DnsAttemptType {
    kUdp = 0,
    kTcpLowEntropy = 1,
    kTcpTruncationRetry = 2,
    kHttp = 3,
    kPlatform = 4,
    kMaxValue = kPlatform,
  };

  class RustFrame {
   public:
    explicit RustFrame(DnsTransactionImpl& transaction) : t(transaction) {}
    ResponseCallback incoming_callback;
    base::TimeTicks attempt_start;
    std::unique_ptr<DnsQuery> query;
    uint64_t Retain(const DnsAttempt* attempt) {
      if (!attempt) return 0;
      auto found = std::ranges::find(handles, attempt);
      if (found != handles.end()) return 1 + std::distance(handles.begin(), found);
      handles.push_back(attempt); return handles.size();
    }
    const DnsAttempt* Get(uint64_t handle) const { return handle ? handles.at(handle - 1) : nullptr; }
    AttemptResult Result(DnsResolverWords result) const { return AttemptResult(result.x, Get(result.a)); }
    DnsResolverWords Run(dns_request_family::TransactionEvent event, DnsResolverWords in = {}) {
      return rustium_dns_request_family_run(4, static_cast<uint32_t>(event), in, this, &Call);
    }
   private:
    using O = dns_request_family::TransactionOp;
    static DnsResolverWords Call(void* context, uint32_t op, DnsResolverWords in) {
      return static_cast<RustFrame*>(context)->Native(static_cast<O>(op), in);
    }
    DnsResolverWords Native(O op, DnsResolverWords in) {
      using dns_request_family::W;
      switch (op) {
        case O::kRead:
          switch (in.x) {
            case 0: return W(!t->callback_.is_null());
            case 1: return W(static_cast<int>(t->attempt_mode_));
            case 2: return W(t->fast_timeout_);
            case 3: return W(t->had_tcp_retry_);
            case 4: return W(t->target_network_ != handles::kInvalidNetworkHandle);
            case 5: return W(t->attempts_.empty());
            case 6: return W(t->session_->udp_tracker()->low_entropy());
            case 8: return W(t->session_->config().append_to_multi_label_name);
            case 9: return W(t->session_->config().ndots);
            case 10: return W(t->attempts_count_for_current_name_);
            case 11: return W(t->qtype_);
            case 13: return W(t->session_->config().attempts);
            case 14: return W(base::FeatureList::IsEnabled(features::kDnsPlatformFailFastAndRetry));
            case 15: return W(features::kDnsPlatformCancelPreviousAttemptOnRetry.Get());
            case 17: return W(t->qnames_.size());
            default: NOTREACHED();
          }
        case O::kWrite:
          switch (in.x) {
            case 1: t->callback_ = std::move(incoming_callback); break;
            case 2: t->had_tcp_retry_ = in.y; break;
            case 3: t->attempts_count_for_current_name_ = in.y; break;
            case 4: t->qnames_initial_size_ = in.y; break;
            default: NOTREACHED();
          }
          return {};
        case O::kCheck:
          switch (in.x) {
            case 0: DCHECK(incoming_callback); DCHECK(t->callback_.is_null()); DCHECK(t->attempts_.empty()); break;
            case 1: CHECK(!t->timer_.IsRunning()); break;
            case 2: DCHECK(in.y); break;
            case 3: DCHECK_GT(t->session_->config().nameservers.size(), 0u); break;
            case 4: DCHECK(!t->session_->config().doh_config.servers().empty()); break;
            case 5: DCHECK(t->dns_server_iterator_); break;
            case 6: DCHECK(Get(in.a)); DCHECK(!t->had_tcp_retry_); break;
            case 7: DCHECK(!t->fast_timeout_); DCHECK(!t->timer_.IsRunning()); DCHECK(!t->callback_.is_null()); break;
            case 8: DCHECK(Get(in.a)); DCHECK(Get(in.a)->GetResponse()); break;
            case 9: DCHECK(Get(in.a) == t->attempts_.back().get()); break;
            case 10: DCHECK_EQ(t->attempt_mode_, DnsTransactionFactory::AttemptMode::kClassic); DCHECK(!t->session_->udp_tracker()->low_entropy()); break;
            case 11: DCHECK_EQ(t->attempt_mode_, DnsTransactionFactory::AttemptMode::kClassic); break;
            case 12: DCHECK(t->dns_server_iterator_->AttemptAvailable()); break;
            case 13: DCHECK_EQ(t->attempt_mode_, DnsTransactionFactory::AttemptMode::kHttp); break;
            case 14: DCHECK(Get(in.a)); break;
            case 15: DCHECK(t->timer_.IsRunning()); break;
            case 16: CHECK_NE(ERR_IO_PENDING, in.y); break;
            case 17: DCHECK_NE(ERR_IO_PENDING, in.y); break;
            case 18: CHECK(in.y); break;
            case 19: DCHECK(!t->attempts_.empty()); break;
            default: NOTREACHED();
          }
          return {};
        case O::kLogStart:
          t->net_log_.BeginEvent(NetLogEventType::DNS_TRANSACTION, [&] { return NetLogStartParams(t->hostname_, t->qtype_); }); return {};
        case O::kLogQuery: {
          auto name = dns_names_util::NetworkToDottedName(t->qnames_.front());
          t->net_log_.BeginEventWithStringParams(NetLogEventType::DNS_TRANSACTION_QUERY, "qname", name.value_or("???MALFORMED_NAME???")); return {};
        }
        case O::kLogQueryEnd: t->net_log_.EndEventWithNetErrorCode(NetLogEventType::DNS_TRANSACTION_QUERY, in.x); return {};
        case O::kLogResponse: t->LogResponse(Get(in.a)); return {};
        case O::kLogEnd: t->net_log_.EndEventWithNetErrorCode(NetLogEventType::DNS_TRANSACTION, in.x); return {};
        case O::kRecordLookup: {
          static constexpr std::array<std::string_view, 3> names{
              "Net.DNS.DnsTransaction.A.LookupResult", "Net.DNS.DnsTransaction.AAAA.LookupResult", "Net.DNS.DnsTransaction.HTTPS.LookupResult"};
          const auto name = names.at(in.x);
          static constexpr std::array<std::string_view, 5> suffixes{"", ".Classic", ".ClassicTruncatedAndTcpRetried", ".DoH", ".Platform"};
          base::UmaHistogramSparse(base::StrCat({name, suffixes.at(in.a)}), in.y);
          return {};
        }
        case O::kRecordAttempt: t->RecordAttemptUma(static_cast<DnsAttemptType>(in.x)); return {};
        case O::kSetTimerRunner: t->timer_.SetTaskRunner(HostResolver::GetTaskRunner(t->request_priority_)); return {};
        case O::kStartElapsed: t->time_from_start_ = std::make_unique<base::ElapsedTimer>(); return {};
        case O::kEncodeHost:
          labeled = dns_names_util::DottedNameToNetwork(t->hostname_, true); return W(labeled.has_value());
        case O::kHostTrailing: return W(t->hostname_.back() == '.');
        case O::kBaseLength: return W(0, labeled->size());
        case O::kEncodedByte: return W(0, labeled->at(in.a));
        case O::kPushBase:
          switch (in.x) {
            case 0: t->qnames_.push_back(*labeled); break;
            case 1: t->qnames_.push_back(std::move(*labeled)); break;
            default: NOTREACHED();
          }
          return {};
        case O::kSearchCount: return W(0, t->session_->config().search.size());
        case O::kEncodeSuffix:
          suffix = dns_names_util::DottedNameToNetwork(base::StrCat({t->hostname_, ".", t->session_->config().search.at(in.a)}), true); return W(suffix.has_value());
        case O::kEncodedLength: return W(0, suffix->size());
        case O::kPushSuffix: t->qnames_.push_back(std::move(*suffix)); return {};
        case O::kQnamesEmpty: return W(t->qnames_.empty());
        case O::kPopQname: t->qnames_.pop_front(); return {};
        case O::kClearAttemptsAll: t->attempts_.clear(); return {};
        case O::kResetIterator:
          switch (in.x) {
            case 0: t->dns_server_iterator_ = t->resolve_context_->GetClassicDnsIterator(t->session_->config(), t->session_.get()); break;
            case 1: t->dns_server_iterator_ = t->resolve_context_->GetDohIterator(t->session_->config(), t->secure_dns_mode_, t->session_.get()); break;
            case 2: t->dns_server_iterator_ = std::make_unique<PlatformDnsServerIterator>(in.y); break;
            default: NOTREACHED();
          }
          return {};
        case O::kIteratorAvailable: return W(t->dns_server_iterator_->AttemptAvailable());
        case O::kIteratorNext: return W(0, t->dns_server_iterator_->GetNextAttemptIndex());
        case O::kNextQueryId: query_id = t->session_->NextQueryId(); return W(query_id);
        case O::kPrepareClassicQuery:
          switch (in.x) {
            case 1: query = std::make_unique<DnsQuery>(query_id, t->qnames_.front(), t->qtype_, t->opt_rdata_); break;
            case 0: query = t->attempts_.front()->GetQuery()->CloneWithNewId(query_id); break;
            default: NOTREACHED();
          }
          return {};
        case O::kCloneRetryQuery: query = Get(in.a)->GetQuery()->CloneWithNewId(query_id); return {};
        case O::kPrepareHttp:
          switch (in.x) {
            case 1: query = std::make_unique<DnsQuery>(0, t->qnames_.front(), t->qtype_, t->opt_rdata_, DnsQuery::PaddingStrategy::BLOCK_LENGTH_128); break;
            case 0: query = std::make_unique<DnsQuery>(*t->attempts_.front()->GetQuery()); break;
            default: NOTREACHED();
          }
          return {};
        case O::kConstructUdp: {
          const auto& config = t->session_->config(); DCHECK_LT(in.a, config.nameservers.size());
          auto socket = t->resolve_context_->url_request_context()->GetNetworkSessionContext()->client_socket_factory->CreateDatagramClientSocket(DatagramSocket::RANDOM_BIND, t->target_network_, t->net_log_.net_log(), t->net_log_.source());
          t->attempts_.push_back(std::make_unique<DnsUDPAttempt>(in.a, std::move(socket), config.nameservers.at(in.a), std::move(query), t->session_->udp_tracker()));
          return W(0, Retain(t->attempts_.back().get()));
        }
        case O::kConstructTcp: {
          const auto& config = t->session_->config(); DCHECK_LT(in.a, config.nameservers.size());
          auto socket = t->resolve_context_->url_request_context()->GetNetworkSessionContext()->client_socket_factory->CreateTransportClientSocket(AddressList(config.nameservers.at(in.a)), t->target_network_, nullptr, nullptr, t->net_log_.net_log(), t->net_log_.source());
          t->attempts_.push_back(std::make_unique<DnsTCPAttempt>(in.a, std::move(socket), std::move(query)));
          return W(0, Retain(t->attempts_.back().get()));
        }
        case O::kConstructHttp:
          AppendDnsHTTPAttempt(t->resolve_context_->GetWeakPtr(), t->session_.get(), in.a, std::move(query), &t.get().attempts_, t->resolve_context_->url_request_context(), t->request_priority_, false);
          return W(0, Retain(t->attempts_.back().get()));
        case O::kConstructPlatform:
          t->attempts_.push_back(t->resolve_context_->url_request_context()->dns_platform_attempt_factory()->CreateDnsPlatformAttempt(in.a, t->qnames_.front(), t->qtype_, t->target_network_, t->net_log_)); return W(0, Retain(t->attempts_.back().get()));
        case O::kStartAttempt: {
          DnsAttempt* attempt = t->attempts_.at(in.b).get(); DCHECK(attempt == Get(in.a));
          const int result = attempt->Start(base::BindOnce(&DnsTransactionImpl::OnAttemptComplete, base::Unretained(&t.get()), static_cast<uint32_t>(in.b), !!in.x, base::TimeTicks::Now()));
          return W(result, in.a);
        }
        case O::kLastAttempt: return W(0, Retain(t->attempts_.back().get()));
        case O::kAttemptAt: DCHECK_LT(in.a, t->attempts_.size()); return W(0, Retain(t->attempts_.at(in.a).get()));
        case O::kAttemptPending: return W(Get(in.a)->IsPending());
        case O::kAttemptResponse: response = Get(in.a)->GetResponse(); return W(!!response);
        case O::kAttemptIsLast: return W(Get(in.a) == t->attempts_.back().get());
        case O::kAttemptServer: return W(0, Get(in.a)->server_index());
        case O::kRecordSuccess: t->resolve_context_->RecordServerSuccess(Get(in.a)->server_index(), t->attempt_mode_, t->session_.get()); return {};
        case O::kRecordFailure: t->resolve_context_->RecordServerFailure(Get(in.a)->server_index(), t->attempt_mode_, in.x, t->session_.get()); return {};
        case O::kRecordRtt: t->resolve_context_->RecordRtt(Get(in.a)->server_index(), t->attempt_mode_, base::TimeTicks::Now() - attempt_start, in.x, t->session_.get()); return {};
        case O::kTimerStop: t->timer_.Stop(); return {};
        case O::kTimerRunning: return W(t->timer_.IsRunning());
        case O::kTimerClassic:
          t->timer_.Start(FROM_HERE, t->resolve_context_->NextClassicFallbackPeriod(in.a, in.b, t->session_.get()), &t.get(), &DnsTransactionImpl::OnFallbackPeriodExpired); return {};
        case O::kTimerHttp:
          t->timer_.Start(FROM_HERE, t->resolve_context_->NextDohFallbackPeriod(in.a, t->session_.get()), &t.get(), &DnsTransactionImpl::OnFallbackPeriodExpired); return {};
        case O::kTimerPlatform:
          t->timer_.Start(FROM_HERE, t->resolve_context_->NextPlatformFallbackPeriod(in.a, in.b, t->session_.get()), &t.get(), &DnsTransactionImpl::OnFallbackPeriodExpired); return {};
        case O::kTimerDouble:
          t->timer_.Start(FROM_HERE, t->timer_.GetCurrentDelay() * in.x, &t.get(), &DnsTransactionImpl::OnFallbackPeriodExpired); return {};
        case O::kTimerTimeout: {
          base::TimeDelta timeout;
          switch (in.x) {
            case 0: timeout = t->resolve_context_->ClassicTransactionTimeout(t->session_.get()); break;
            case 1: timeout = t->resolve_context_->SecureTransactionTimeout(t->secure_dns_mode_, t->session_.get()); break;
            case 2: timeout = t->resolve_context_->PlatformTransactionTimeout(t->session_.get()); break;
            default: NOTREACHED();
          }
          timeout -= t->time_from_start_->Elapsed();
          t->timer_.Start(FROM_HERE, timeout, &t.get(), &DnsTransactionImpl::OnTimeout); return {};
        }
        case O::kClearErase: t->attempts_.erase(std::next(t->attempts_.begin(), in.a)); return {};
        case O::kAttemptCount: return W(0, t->attempts_.size());
        case O::kSetDohDetails: t->doh_details_ = Get(in.a)->GetDohResolutionDetails(); return {};
        case O::kRunCallback: std::move(t->callback_).Run(in.x, response); return {};
        case O::kPostCallback:
          HostResolver::GetTaskRunner(t->request_priority_)->PostTask(FROM_HERE, base::BindOnce(&DnsTransactionImpl::DoCallback, t->weak_ptr_factory_.GetWeakPtr(), Result(in))); return {};
        case O::kLogAttempt: {
          const DnsAttempt* attempt = Get(in.a);
          switch (in.x) {
            case 0: t->net_log_.AddEventReferencingSource(NetLogEventType::DNS_TRANSACTION_ATTEMPT, attempt->GetSocketNetLog().source()); break;
            case 1:
              t->net_log_.AddEventReferencingSource(NetLogEventType::DNS_TRANSACTION_HTTPS_ATTEMPT, attempt->GetSocketNetLog().source());
              attempt->GetSocketNetLog().AddEventReferencingSource(NetLogEventType::DNS_TRANSACTION_HTTPS_ATTEMPT, t->net_log_.source()); break;
            case 2: t->net_log_.AddEventReferencingSource(NetLogEventType::DNS_TRANSACTION_TCP_ATTEMPT, attempt->GetSocketNetLog().source()); break;
            default: NOTREACHED();
          }
          return {};
        }
        default: NOTREACHED();
      }
    }
    raw_ref<DnsTransactionImpl> t;
    std::vector<const DnsAttempt*> handles;
    std::optional<std::vector<uint8_t>> labeled;
    std::optional<std::vector<uint8_t>> suffix;
    uint16_t query_id = 0;
    raw_ptr<const DnsResponse> response = nullptr;
  };

  // Prepares |qnames_| according to the DnsConfig.
  int PrepareSearch() {
    RustFrame frame(*this);
    return frame.Run(dns_request_family::TransactionEvent::kPrepare).x;
}

  void DoCallback(AttemptResult result) {
    RustFrame frame(*this);
    frame.Run(dns_request_family::TransactionEvent::kCallback, dns_request_family::W(result.rv, frame.Retain(result.attempt)));
}

  void RecordAttemptUma(DnsAttemptType attempt_type) {
    UMA_HISTOGRAM_ENUMERATION("Net.DNS.DnsTransaction.AttemptType",
                              attempt_type);
  }

  void RecordLookupResult(int net_error) {
    RustFrame frame(*this);
    frame.Run(dns_request_family::TransactionEvent::kRecordLookup, dns_request_family::W(net_error));
}

  AttemptResult MakeAttempt() {
    RustFrame frame(*this);
    return frame.Result(frame.Run(dns_request_family::TransactionEvent::kAttempt));
}

  AttemptResult MakeClassicDnsAttempt() {
    RustFrame frame(*this);
    return frame.Result(frame.Run(dns_request_family::TransactionEvent::kClassic));
}

  // Makes another attempt at the current name, |qnames_.front()|, using the
  // next nameserver.
  AttemptResult MakeUdpAttempt(size_t server_index,
                               std::unique_ptr<DnsQuery> query) {
    RustFrame frame(*this); frame.query = std::move(query);
    return frame.Result(frame.Run(dns_request_family::TransactionEvent::kUdp, dns_request_family::W(0, server_index)));
}

  AttemptResult MakeHTTPAttempt() {
    RustFrame frame(*this);
    return frame.Result(frame.Run(dns_request_family::TransactionEvent::kHttp));
}

  AttemptResult RetryUdpAttemptAsTcp(const DnsAttempt* previous_attempt) {
    RustFrame frame(*this);
    return frame.Result(frame.Run(dns_request_family::TransactionEvent::kRetryTcp, dns_request_family::W(0, frame.Retain(previous_attempt))));
}

  AttemptResult MakeTcpAttempt(size_t server_index,
                               std::unique_ptr<DnsQuery> query) {
    RustFrame frame(*this); frame.query = std::move(query);
    return frame.Result(frame.Run(dns_request_family::TransactionEvent::kTcp, dns_request_family::W(0, server_index)));
}

  // Begins query for the current name. Makes the first attempt.
  AttemptResult StartQuery() {
    RustFrame frame(*this);
    return frame.Result(frame.Run(dns_request_family::TransactionEvent::kQuery));
}

  void OnAttemptComplete(uint32_t attempt_number,
                         bool record_rtt,
                         base::TimeTicks start,
                         int rv) {
    RustFrame frame(*this); frame.attempt_start = start;
    frame.Run(dns_request_family::TransactionEvent::kComplete, dns_request_family::W(rv, attempt_number, record_rtt));
}

  void LogResponse(const DnsAttempt* attempt) {
    if (attempt) {
      net_log_.AddEvent(NetLogEventType::DNS_TRANSACTION_RESPONSE,
                        [&](NetLogCaptureMode capture_mode) {
                          return attempt->NetLogResponseParams(capture_mode);
                        });
    }
  }

  bool MoreAttemptsAllowed() const {
    struct View { raw_ref<const DnsTransactionImpl> transaction; } view{raw_ref(*this)};
    auto read = [](void* context, uint32_t operation, DnsResolverWords input) {
      const auto& owner = static_cast<View*>(context)->transaction.get();
      using O = dns_request_family::TransactionOp;
      switch (static_cast<O>(operation)) {
        case O::kRead: CHECK_EQ(input.x, 3); return dns_request_family::W(owner.had_tcp_retry_);
        case O::kIteratorAvailable: return dns_request_family::W(owner.dns_server_iterator_->AttemptAvailable());
        default: NOTREACHED();
      }
    };
    return rustium_dns_request_family_run(4, static_cast<uint32_t>(dns_request_family::TransactionEvent::kMore), {}, &view, read).x;
}

  // Resolves the result of a DnsAttempt until a terminal result is reached
  // or it will complete asynchronously (ERR_IO_PENDING).
  AttemptResult ProcessAttemptResult(AttemptResult result) {
    RustFrame frame(*this);
    return frame.Result(frame.Run(dns_request_family::TransactionEvent::kProcess, dns_request_family::W(result.rv, frame.Retain(result.attempt))));
}

  // Clears and cancels all pending attempts. If |leave_attempt| is not
  // null, that attempt is not cleared even if pending.
  void ClearAttempts(const DnsAttempt* leave_attempt) {
    RustFrame frame(*this);
    frame.Run(dns_request_family::TransactionEvent::kClear, dns_request_family::W(0, frame.Retain(leave_attempt)));
}

  bool AnyAttemptPending() {
    RustFrame frame(*this);
    return frame.Run(dns_request_family::TransactionEvent::kPending).x;
}

  void OnFallbackPeriodExpired() {
    RustFrame frame(*this);
    frame.Run(dns_request_family::TransactionEvent::kFallback);
}

  void StartTimeoutTimer() {
    RustFrame frame(*this);
    frame.Run(dns_request_family::TransactionEvent::kTimeoutTimer);
}

  void OnTimeout() {
    RustFrame frame(*this);
    frame.Run(dns_request_family::TransactionEvent::kTimeout);
}

  AttemptResult MakePlatformAttempt() {
    RustFrame frame(*this);
    return frame.Result(frame.Run(dns_request_family::TransactionEvent::kPlatform));
}

  scoped_refptr<DnsSession> session_;
  std::string hostname_;
  uint16_t qtype_;
  raw_ptr<const OptRecordRdata, DanglingUntriaged> opt_rdata_;
  const DnsTransactionFactory::AttemptMode attempt_mode_;
  const SecureDnsMode secure_dns_mode_;
  const handles::NetworkHandle target_network_ = handles::kInvalidNetworkHandle;
  // Cleared in DoCallback.
  ResponseCallback callback_;

  // When true, transaction should time out immediately on expiration of the
  // last attempt fallback period rather than waiting the overall transaction
  // timeout period.
  const bool fast_timeout_;

  NetLogWithSource net_log_;

  // Search list of fully-qualified DNS names to query next (in DNS format).
  base::circular_deque<std::vector<uint8_t>> qnames_;
  size_t qnames_initial_size_ = 0;

  // List of attempts for the current name. Reset in StartQuery(). Note:
  // if kDnsPlatformCancelPreviousAttemptOnRetry is enabled, kPlatform clears
  // pending attempts in this list every time it makes a new attempt.
  std::vector<std::unique_ptr<DnsAttempt>> attempts_;
  // Count of attempts for the current name. Reset in StartQuery().
  int attempts_count_for_current_name_ = 0;

  // Records when an attempt was retried via TCP due to a truncation error.
  bool had_tcp_retry_ = false;

  base::OneShotTimer timer_;
  std::unique_ptr<base::ElapsedTimer> time_from_start_;

  base::SafeRef<ResolveContext> resolve_context_;

  // Iterator to get the index of the DNS server for each search query.
  // Declared after `resolve_context_` so `resolve_context_` outlives the iterator.
  std::unique_ptr<DnsServerIterator> dns_server_iterator_;

  RequestPriority request_priority_ = DEFAULT_PRIORITY;

  THREAD_CHECKER(thread_checker_);

  std::optional<DohResolutionDetails> doh_details_;

  base::WeakPtrFactory<DnsTransactionImpl> weak_ptr_factory_{this};
};

// ----------------------------------------------------------------------------

// Implementation of DnsTransactionFactory that returns instances of
// DnsTransactionImpl.
class DnsTransactionFactoryImpl : public DnsTransactionFactory {
 public:
  explicit DnsTransactionFactoryImpl(DnsSession* session) {
    session_ = session;
  }

  std::unique_ptr<DnsTransaction> CreateTransaction(
      std::string hostname,
      uint16_t qtype,
      const NetLogWithSource& net_log,
      AttemptMode attempt_mode,
      SecureDnsMode secure_dns_mode,
      handles::NetworkHandle target_network,
      ResolveContext* resolve_context,
      bool fast_timeout) override {
    return std::make_unique<DnsTransactionImpl>(
        session_.get(), std::move(hostname), qtype, net_log,
        // No factory-level EDNS option injection; per-transaction options are
        // passed through other call sites when needed.
        /*opt_rdata=*/nullptr, attempt_mode, secure_dns_mode, target_network,
        resolve_context, fast_timeout);
  }

  std::unique_ptr<DnsProbeRunner> CreateDohProbeRunner(
      ResolveContext* resolve_context) override {
    // Start a timer that will emit metrics after a timeout to indicate whether
    // DoH auto-upgrade was successful for this session.
    resolve_context->StartDohAutoupgradeSuccessTimer(session_.get());

    return std::make_unique<DnsOverHttpsProbeRunner>(
        session_->GetWeakPtr(), resolve_context->GetWeakPtr());
  }

  SecureDnsMode GetSecureDnsModeForTest() override {
    return session_->config().secure_dns_mode;
  }

 private:
  scoped_refptr<DnsSession> session_;
};

}  // namespace

DnsTransactionFactory::DnsTransactionFactory() = default;
DnsTransactionFactory::~DnsTransactionFactory() = default;

// static
std::unique_ptr<DnsTransactionFactory> DnsTransactionFactory::CreateFactory(
    DnsSession* session) {
  return std::make_unique<DnsTransactionFactoryImpl>(session);
}

}  // namespace net
