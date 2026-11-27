#include "net/http/http_response_info_rust_original_vary.h"

#include "base/pickle.h"
namespace net {
HttpResponseInfoOriginalVaryData::HttpResponseInfoOriginalVaryData() = default;
bool HttpResponseInfoOriginalVaryData::InitFromPickle(
    base::PickleIterator* iter) {
  is_valid_ = false;

  // Create a copy of the iterator to probe for the old format. This avoids
  // advancing `iter` before we know which format we are parsing.
  base::PickleIterator probe_iter(*iter);
  std::optional<base::span<const uint8_t>> bytes_for_probe =
      probe_iter.ReadBytes(crypto::obsolete::Md5::kSize);

  // Support for old cache entries that stored a raw 16-byte MD5 digest.
  // We detect 16-byte MD5 hash by checking if the pickle's total size is
  // exactly 16 bytes.
  if (bytes_for_probe && !probe_iter.ReadBytes(1).has_value()) {
    // It's an old-style entry. Read the data from the original iterator to
    // consume it.
    std::optional<base::span<const uint8_t>> bytes =
        iter->ReadBytes(crypto::obsolete::Md5::kSize);
    Md5Hash md5_hash;
    base::span(md5_hash).copy_from(*bytes);
    hash_ = md5_hash;
    return is_valid_ = true;
  }

  // It's a new-style entry, so we parse it from the original `iter`.
  // New format: HashType enum followed by the digest.
  int read_hash_type;
  if (!iter->ReadInt(&read_hash_type)) {
    return false;
  }

  if (read_hash_type == static_cast<int>(HashType::kSHA256)) {
    std::optional<base::span<const uint8_t>> bytes =
        iter->ReadBytes(crypto::hash::kSha256Size);
    if (!bytes) {
      return false;
    }
    Sha256Hash sha_hash;
    base::span(sha_hash).copy_from(*bytes);
    hash_ = sha_hash;
  } else if (read_hash_type == static_cast<int>(HashType::kMD5)) {
    std::optional<base::span<const uint8_t>> bytes =
        iter->ReadBytes(crypto::obsolete::Md5::kSize);
    if (!bytes) {
      return false;
    }
    Md5Hash md5_hash;
    base::span(md5_hash).copy_from(*bytes);
    hash_ = md5_hash;
  } else {
    return false;  // Invalid hash type.
  }

  return is_valid_ = true;
}

void HttpResponseInfoOriginalVaryData::Persist(base::Pickle* pickle) const {
  DCHECK(is_valid());
  std::visit(
      [&](const auto& hash) {
        pickle->WriteInt(static_cast<int>(hash_type()));
        pickle->WriteBytes(hash);
      },
      hash_);
}

}  // namespace net
