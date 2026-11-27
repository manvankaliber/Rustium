#![forbid(unsafe_code)]
use crate::connection_jobs_rust_core::{Controller, Module, Services};
use crate::errors::*;
#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Operation {
    Construct = 0,
}
#[repr(u32)]
#[derive(Clone, Copy, Debug)]
pub enum Action {
    CheckValid = 0,
    PushRoot = 1,
    DestinationScheme = 2,
    SchemeCrypto = 3,
    SchemelessSsl = 4,
    EndpointAlpnMode = 5,
    AllowedCerts = 6,
    Privacy = 7,
    SetDestinationAlpn = 8,
    ClearAlpn = 9,
    AlpnHttp11 = 10,
    AlpnAll = 11,
    AlpnApplications = 12,
    CheckAll = 13,
    HasProperties = 14,
    MaybeForceHttp11 = 15,
    RenegoDefault = 16,
    RenegoHttp11 = 17,
    DestinationCertFetch = 18,
    EarlyData = 19,
    DestinationUsage = 20,
    Direct = 21,
    ForceTunnel = 22,
    GetToProxyAllowed = 23,
    ChainLength = 24,
    EndpointPair = 25,
    SchemeTransport = 26,
    PairTransport = 27,
    BeginProtocols = 28,
    ProtocolCount = 29,
    InsertProtocol = 30,
    BuildDirect = 31,
    PushProxy = 32,
    EmptyNak = 33,
    EndpointNak = 34,
    ProxySecure = 35,
    ProxyQuic = 36,
    ProxyHttp = 37,
    ProxySocks = 38,
    ProxySocks5 = 39,
    CheckProxyQuicAt = 40,
    ProxyDisableFetch = 41,
    ProxyAlpnEndpoint = 42,
    ProxyUsage = 43,
    ProxyTransport = 44,
    NextProxyHost = 45,
    WrapProxySsl = 46,
    BuildQuic = 47,
    CheckNotQuic = 48,
    BuildHttp = 49,
    BuildSocks = 50,
    PopProxy = 51,
    WrapDestinationSsl = 52,
    Return = 53,
}

type C<'a, S> = Controller<'a, S>;
macro_rules! v {
    ($c:expr,$a:ident) => {$c.value(Action::$a as u32,0)};
}
macro_rules! s {
    ($c:expr,$a:ident) => {$c.step(Action::$a as u32,0)};
    ($c:expr,$a:ident,$x:expr) => {$c.step(Action::$a as u32,$x)};
}
fn using_ssl<S: Services>(c: &mut C<'_, S>) -> bool {
    if v!(c,DestinationScheme) != 0 {
        v!(c,SchemeCrypto) != 0
    } else {
        v!(c,SchemelessSsl) != 0
    }
}
fn alpn<S: Services>(c: &mut C<'_, S>, mode: i64, renego: bool) {
    if mode == 0 {
        s!(c,ClearAlpn);
        return;
    }
    if mode == 1 {
        s!(c,AlpnHttp11);
        s!(c,AlpnApplications);
    } else {
        s!(c,CheckAll,mode);
        s!(c,AlpnAll);
        s!(c,AlpnApplications);
        if v!(c,HasProperties) != 0 {
            s!(c,MaybeForceHttp11);
        }
    }
    s!(c,RenegoDefault,i64::from(renego));
    if renego {
        s!(c,RenegoHttp11);
    }
}
fn protocols<S: Services>(c: &mut C<'_, S>) {
    s!(c,BeginProtocols);
    for i in 0..v!(c,ProtocolCount) {
        s!(c,InsertProtocol,i);
    }
}
fn proxy<S: Services>(c: &mut C<'_, S>, index: i64, tunnel: bool) {
    s!(c,PushProxy,index);
    if index < v!(c,ChainLength) - 1 {
        s!(c,EmptyNak);
    } else {
        s!(c,EndpointNak);
    }
    if v!(c,ProxySecure) != 0 {
        s!(c,ProxyDisableFetch);
        s!(c,ProxyAlpnEndpoint);
        alpn(c, 2, false);
        s!(c,ProxyUsage);
    }
    if v!(c,ProxyQuic) != 0 {
        for previous in 0..index {
            s!(c,CheckProxyQuicAt,previous);
        }
        s!(c,BuildQuic,i64::from(tunnel));
        s!(c,PopProxy);
        return;
    }
    if index == 0 {
        protocols(c);
        s!(c,ProxyTransport);
    } else {
        s!(c,NextProxyHost);
        proxy(c, index - 1, true);
    }
    if v!(c,ProxySecure) != 0 {
        s!(c,WrapProxySsl);
    }
    if v!(c,ProxyHttp) != 0 {
        s!(c,CheckNotQuic);
        s!(c,BuildHttp,i64::from(tunnel));
    } else {
        s!(c,ProxySocks);
        let socks5 = v!(c,ProxySocks5);
        s!(c,BuildSocks,socks5);
    }
    s!(c,PopProxy);
}
pub fn run<S: Services>(op: u32, _arg: i64, native: &mut S) -> i64 {
    if op != 0 {
        return ERR_UNEXPECTED as i64;
    }
    let mut c = Controller::new(native, Module::ParamsFactory);
    s!(c,CheckValid);
    s!(c,PushRoot);
    if using_ssl(&mut c) {
        s!(c,AllowedCerts);
        s!(c,Privacy);
        let mode = v!(c,EndpointAlpnMode);
        if mode != 0 {
            s!(c,SetDestinationAlpn);
        }
        alpn(&mut c, mode, true);
        s!(c,DestinationCertFetch);
        s!(c,EarlyData);
        s!(c,DestinationUsage);
    }
    if v!(c,Direct) != 0 {
        if v!(c,DestinationScheme) != 0 {
            s!(c,SchemeTransport);
        } else {
            s!(c,PairTransport);
        }
        protocols(&mut c);
        s!(c,BuildDirect);
    } else {
        let tunnel = v!(c,ForceTunnel) != 0 || using_ssl(&mut c) || v!(c,GetToProxyAllowed) == 0;
        s!(c,EndpointPair);
        let last = v!(c,ChainLength) - 1;
        proxy(&mut c, last, tunnel);
    }
    if using_ssl(&mut c) {
        s!(c,WrapDestinationSsl);
    }
    s!(c,Return);
    0
}
