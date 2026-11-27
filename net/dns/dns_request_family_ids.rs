// Generated from make_abi.py. Draft operation IDs; no runtime validation.
#[allow(non_snake_case, non_upper_case_globals)]
pub mod RequestEvent {
    pub const Destroy: u32 = 0;
    pub const Start: u32 = 1;
    pub const Loop: u32 = 2;
    pub const IPv6: u32 = 3;
    pub const Parameters: u32 = 4;
    pub const ParametersComplete: u32 = 5;
    pub const ResolveLocal: u32 = 6;
    pub const StartJob: u32 = 7;
    pub const Finish: u32 = 8;
    pub const IOComplete: u32 = 9;
    pub const ChangePriority: u32 = 10;
    pub const Cancelled: u32 = 11;
    pub const Completed: u32 = 12;
    pub const FixResults: u32 = 13;
}
#[allow(non_snake_case, non_upper_case_globals)]
pub mod RequestOp {
    pub const Read: u32 = 0;
    pub const Write: u32 = 1;
    pub const Check: u32 = 2;
    pub const LogStart: u32 = 3;
    pub const LogFinish: u32 = 4;
    pub const LogCancel: u32 = 5;
    pub const StartIPv6: u32 = 6;
    pub const InitializeKey: u32 = 7;
    pub const Nat64Facts: u32 = 8;
    pub const LastIPv6: u32 = 9;
    pub const StartGlobal: u32 = 10;
    pub const ResolveLocal: u32 = 11;
    pub const LocalError: u32 = 12;
    pub const HasStale: u32 = 13;
    pub const CopyLocalResults: u32 = 14;
    pub const MoveStale: u32 = 15;
    pub const StartJob: u32 = 16;
    pub const Squash: u32 = 17;
    pub const RunCallback: u32 = 18;
    pub const CancelJob: u32 = 19;
    pub const ChangeJobPriority: u32 = 20;
    pub const ResetCallback: u32 = 21;
    pub const ResetJob: u32 = 22;
    pub const ResetResolver: u32 = 23;
    pub const CopyEndpoints: u32 = 24;
    pub const CopyAliases: u32 = 25;
    pub const FixAliases: u32 = 26;
    pub const CopyLegacy: u32 = 27;
    pub const ReleaseLocal: u32 = 28;
}
#[allow(non_snake_case, non_upper_case_globals)]
pub mod EndpointEvent {
    pub const Destroy: u32 = 0;
    pub const Start: u32 = 1;
    pub const Loop: u32 = 2;
    pub const IPv6: u32 = 3;
    pub const IPv6Complete: u32 = 4;
    pub const ResolveLocal: u32 = 5;
    pub const StartJob: u32 = 6;
    pub const IOComplete: u32 = 7;
    pub const ChangePriority: u32 = 8;
    pub const Completed: u32 = 9;
    pub const Cancelled: u32 = 10;
    pub const Changed: u32 = 11;
    pub const Finalize: u32 = 12;
    pub const ClearStale: u32 = 13;
    pub const Notify: u32 = 14;
    pub const EndpointSource: u32 = 15;
    pub const AliasSource: u32 = 16;
    pub const CryptoReady: u32 = 17;
    pub const IsStaleRefreshing: u32 = 18;
    pub const Debug: u32 = 19;
}
#[allow(non_snake_case, non_upper_case_globals)]
pub mod EndpointOp {
    pub const Read: u32 = 0;
    pub const Write: u32 = 1;
    pub const Check: u32 = 2;
    pub const CancelJob: u32 = 3;
    pub const LogCancel: u32 = 4;
    pub const ResetJob: u32 = 5;
    pub const RunFinished: u32 = 6;
    pub const RunUpdated: u32 = 7;
    pub const InitializeKey: u32 = 8;
    pub const ResolveLocal: u32 = 9;
    pub const LocalError: u32 = 10;
    pub const FreshError: u32 = 11;
    pub const ClearTasks: u32 = 12;
    pub const ResolveFresh: u32 = 13;
    pub const AdoptFresh: u32 = 14;
    pub const CurrentNetwork: u32 = 15;
    pub const ResultNetwork: u32 = 16;
    pub const ConvertStale: u32 = 17;
    pub const ResetStale: u32 = 18;
    pub const LogStale: u32 = 19;
    pub const PostStale: u32 = 20;
    pub const StartJob: u32 = 21;
    pub const FinalizeResult: u32 = 22;
    pub const MakeEmpty: u32 = 23;
    pub const Squash: u32 = 24;
    pub const SetError: u32 = 25;
    pub const SetDetails: u32 = 26;
    pub const HasTaskManager: u32 = 27;
    pub const GetCryptoReady: u32 = 28;
    pub const StartIPv6: u32 = 29;
    pub const ChangeJobPriority: u32 = 30;
    pub const DebugTaskCount: u32 = 31;
    pub const DebugTask: u32 = 32;
    pub const DebugAppend: u32 = 33;
    pub const ReleaseLocal: u32 = 34;
}
#[allow(non_snake_case, non_upper_case_globals)]
pub mod ClientEvent {
    pub const Secure: u32 = 0;
    pub const Insecure: u32 = 1;
    pub const Additional: u32 = 2;
    pub const Mode: u32 = 3;
    pub const SecureFallback: u32 = 4;
    pub const InsecureFallback: u32 = 5;
    pub const SetSystem: u32 = 6;
    pub const SetOverrides: u32 = 7;
    pub const ReplaceSession: u32 = 8;
    pub const Preset: u32 = 9;
    pub const Build: u32 = 10;
    pub const Update: u32 = 11;
    pub const Upgrade: u32 = 12;
    pub const ConfigLog: u32 = 13;
    pub const SetInsecure: u32 = 14;
    pub const IncrementFailures: u32 = 15;
    pub const ClearFailures: u32 = 16;
    pub const UsePlatform: u32 = 17;
    pub const Session: u32 = 18;
}
#[allow(non_snake_case, non_upper_case_globals)]
pub mod ClientOp {
    pub const Read: u32 = 0;
    pub const Write: u32 = 1;
    pub const Check: u32 = 2;
    pub const SystemSame: u32 = 3;
    pub const StoreSystem: u32 = 4;
    pub const OverridesSame: u32 = 5;
    pub const StoreOverrides: u32 = 6;
    pub const ApplyOverrides: u32 = 7;
    pub const HasOverridesAll: u32 = 8;
    pub const ClearNameservers: u32 = 9;
    pub const UpgradeDot: u32 = 10;
    pub const UpgradeNameservers: u32 = 11;
    pub const NameserverCount: u32 = 12;
    pub const NameserverPublic: u32 = 13;
    pub const NameserverLoopback: u32 = 14;
    pub const Feature: u32 = 15;
    pub const Metric: u32 = 16;
    pub const ClearFactory: u32 = 17;
    pub const NewSession: u32 = 18;
    pub const NewFactory: u32 = 19;
    pub const ConfigSame: u32 = 20;
    pub const LogChanged: u32 = 21;
    pub const PresetCount: u32 = 22;
    pub const PresetMatches: u32 = 23;
    pub const PresetGroupCount: u32 = 24;
    pub const PresetAddressCount: u32 = 25;
    pub const PresetAppend: u32 = 26;
    pub const PrepareLog: u32 = 27;
    pub const SetLogBool: u32 = 28;
}
#[allow(non_snake_case, non_upper_case_globals)]
pub mod TransactionEvent {
    pub const Destroy: u32 = 0;
    pub const Start: u32 = 1;
    pub const Prepare: u32 = 2;
    pub const Callback: u32 = 3;
    pub const Attempt: u32 = 4;
    pub const Classic: u32 = 5;
    pub const Udp: u32 = 6;
    pub const Http: u32 = 7;
    pub const RetryTcp: u32 = 8;
    pub const Tcp: u32 = 9;
    pub const Query: u32 = 10;
    pub const Complete: u32 = 11;
    pub const More: u32 = 12;
    pub const Process: u32 = 13;
    pub const Clear: u32 = 14;
    pub const Pending: u32 = 15;
    pub const Fallback: u32 = 16;
    pub const TimeoutTimer: u32 = 17;
    pub const Timeout: u32 = 18;
    pub const Platform: u32 = 19;
    pub const RecordLookup: u32 = 20;
}
#[allow(non_snake_case, non_upper_case_globals)]
pub mod TransactionOp {
    pub const Read: u32 = 0;
    pub const Write: u32 = 1;
    pub const Check: u32 = 2;
    pub const LogStart: u32 = 3;
    pub const LogQuery: u32 = 4;
    pub const LogQueryEnd: u32 = 5;
    pub const LogResponse: u32 = 6;
    pub const LogEnd: u32 = 7;
    pub const RecordLookup: u32 = 8;
    pub const RecordAttempt: u32 = 9;
    pub const SetTimerRunner: u32 = 10;
    pub const StartElapsed: u32 = 11;
    pub const EncodeHost: u32 = 12;
    pub const HostTrailing: u32 = 13;
    pub const CountLabels: u32 = 14;
    pub const PushBase: u32 = 15;
    pub const SearchCount: u32 = 16;
    pub const EncodeSuffix: u32 = 17;
    pub const EncodedLength: u32 = 18;
    pub const BaseLength: u32 = 19;
    pub const PushSuffix: u32 = 20;
    pub const QnamesEmpty: u32 = 21;
    pub const PopQname: u32 = 22;
    pub const ClearAttemptsAll: u32 = 23;
    pub const ResetIterator: u32 = 24;
    pub const IteratorAvailable: u32 = 25;
    pub const IteratorNext: u32 = 26;
    pub const NextQueryId: u32 = 27;
    pub const PrepareClassicQuery: u32 = 28;
    pub const CloneRetryQuery: u32 = 29;
    pub const PrepareHttp: u32 = 30;
    pub const ConstructUdp: u32 = 31;
    pub const ConstructTcp: u32 = 32;
    pub const ConstructHttp: u32 = 33;
    pub const ConstructPlatform: u32 = 34;
    pub const StartAttempt: u32 = 35;
    pub const LastAttempt: u32 = 36;
    pub const AttemptAt: u32 = 37;
    pub const AttemptPending: u32 = 38;
    pub const AttemptResponse: u32 = 39;
    pub const AttemptIsLast: u32 = 40;
    pub const RecordSuccess: u32 = 41;
    pub const RecordFailure: u32 = 42;
    pub const RecordRtt: u32 = 43;
    pub const TimerStop: u32 = 44;
    pub const TimerRunning: u32 = 45;
    pub const TimerClassic: u32 = 46;
    pub const TimerHttp: u32 = 47;
    pub const TimerPlatform: u32 = 48;
    pub const TimerDouble: u32 = 49;
    pub const TimerTimeout: u32 = 50;
    pub const ClearErase: u32 = 51;
    pub const AttemptCount: u32 = 52;
    pub const CurrentNameCount: u32 = 53;
    pub const ClearCursorAdvance: u32 = 54;
    pub const CallbackPresent: u32 = 55;
    pub const SetDohDetails: u32 = 56;
    pub const RunCallback: u32 = 57;
    pub const PostCallback: u32 = 58;
    pub const CurrentElapsed: u32 = 59;
    pub const EncodedByte: u32 = 60;
    pub const AttemptServer: u32 = 61;
    pub const LogAttempt: u32 = 62;
}
#[allow(non_snake_case, non_upper_case_globals)]
pub mod ProbeEvent {
    pub const Initialize: u32 = 0;
    pub const Start: u32 = 1;
    pub const Delay: u32 = 2;
    pub const Continue: u32 = 3;
    pub const Complete: u32 = 4;
}
#[allow(non_snake_case, non_upper_case_globals)]
pub mod ProbeOp {
    pub const Read: u32 = 0;
    pub const Check: u32 = 1;
    pub const EncodeProbe: u32 = 2;
    pub const Resize: u32 = 3;
    pub const StatsPresent: u32 = 4;
    pub const NewStats: u32 = 5;
    pub const StatsClear: u32 = 6;
    pub const ClearAll: u32 = 7;
    pub const ClockNow: u32 = 8;
    pub const StartSequence: u32 = 9;
    pub const BackoffFailure: u32 = 10;
    pub const PostContinue: u32 = 11;
    pub const ConstructHttp: u32 = 12;
    pub const StartAttempt: u32 = 13;
    pub const RecordFailure: u32 = 14;
    pub const ExtractResponse: u32 = 15;
    pub const ResultCount: u32 = 16;
    pub const ResultIsData: u32 = 17;
    pub const ResultHasEndpoints: u32 = 18;
    pub const RecordSuccess: u32 = 19;
    pub const RecordRtt: u32 = 20;
    pub const RecordSequence: u32 = 21;
    pub const Delay: u32 = 22;
    pub const ResponsePresent: u32 = 23;
}
