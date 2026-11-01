# ETHNIR login: "curl 28, HTTP 0" connection timeout

## What the message means

`curl 28` is `CURLE_OPERATION_TIMEDOUT` and `HTTP 0` means no HTTP response was ever
received. The failure is a TCP connection to the license host that never completed - it
happens before TLS and before the key is looked at. Nothing about the license key is
wrong; the key was never submitted.

## Measured cause: the panel host drops most connections

From an external network on 2026-10-31:

- `ethnir.x10.mx` and `www.ethnir.x10.mx` both resolve to `198.91.81.11`. Only IPv4.
- A burst of ten HTTPS attempts against `https://ethnir.x10.mx/connect` produced one
  connection; nine timed out inside the TCP handshake.
- When a connection did land, the answer was `HTTP/2 200` from `LiteSpeed` / `PHP 7.4.33`
  with `content-type: application/json`, so the panel, the route and the response
  format are intact.
- `www.ethnir.x10.mx` returned the panel's own JSON (`{"status":false,"reason":"Unknown
  client"}` without the `game` field), i.e. the same application, also reachable only
  intermittently.
- Two independent public fetchers failed as well: `api.codetabs.com/v1/proxy` returned
  `error code: 522` (Cloudflare: origin unreachable) and `r.jina.ai` timed out. The
  failure is therefore not specific to the phone or to one network path.
- `shaii.x10.mx` (same hosting family, `198.91.81.14`) answered on every attempt, so this
  is not an x10-wide outage. It is specific to the `ethnir` account's server, which is
  dropping connection attempts in bursts.

Three runs of the new client retry logic against the live endpoint, minutes apart:

```
error=0  status=200 attempts=3   (succeeded on the third attempt)
error=28 status=0   attempts=4   (all four attempts timed out)
error=28 status=0   attempts=4   (all four attempts timed out)
```

The retry rescues the burst where the host answers after a drop, and cannot rescue the
periods where it answers nothing. Those periods are the actual outage and only the host
can end them, so the client change is a mitigation, not a repair.

The host itself needs attention. The client cannot make a server answer, but it can
stop turning one dropped connection into a failed login.

## Client change

- `ImGui3SignedSession.h`: the panel is published under two names of the same account,
  so `kEndpoints[]` holds the canonical `https://ethnir.x10.mx/connect` and the alias
  `https://www.ethnir.x10.mx/connect`.
- `Jni_Stuff.h`: the single request is now `PerformLicensePost(endpoint, ...)` with a
  fresh easy handle per attempt (a handle that saw a dead connection is not reused).
- `Login` walks both names with up to two attempts each, 5 s connect timeout and 12 s
  total per attempt, instead of one 10 s / 20 s attempt that gave up on the first drop.
- Only transport failures are retried: `CURLE_OPERATION_TIMEDOUT`,
  `CURLE_COULDNT_CONNECT`, `CURLE_COULDNT_RESOLVE_HOST`, `CURLE_GOT_NOTHING`. A TLS
  verdict, a redirect, an HTTP status or a signed rejection is final and is never
  retried.
- Redirects are still refused, host and peer verification are still on, and the signed
  session is verified exactly as before. No verification was relaxed.
- The failure text now reports the attempt count:
  `License server unreachable (curl 28, HTTP 0, 4 attempts): ...`.

Worst case a login now takes about 20 s before reporting failure instead of 10 s, and
it succeeds whenever one of the four attempts gets through. The call already runs on a
background thread (`std::async` in `ApplicationUI.h`), so the panel does not freeze.

## Checks

- The new `PerformLicensePost` and the retry loop were extracted verbatim into a harness
  and compiled against the project's libcurl headers with `-Wall -Wextra -Werror`, then
  run against the live endpoint: `error=0 status=200 attempts=1` when the host was up.
- Brace/paren balance of both edited headers matches the pre-edit files.
- `tests/test_signed_session.cpp` and `tests/test_ethnir_auth_reply.cpp` could not be
  rebuilt in this environment: the OpenSSL development headers are not installed and
  installing them needs root. Run them where `libssl-dev` is available.
- `Jni_Stuff.h` has no host test; the JNI and Android TLS parts were not touched.

## Not verified

- On-device login. The host is intermittent, so a test may pass or fail depending on
  whether a connection lands during the four attempts.
- Whether the host's drops are firewall rate limiting (repeated attempts from one IP) or
  an overloaded shared server. If they are rate limiting, four attempts per login is
  still below a typical threshold, but the server log is the place to confirm.
- The `www` name presenting a certificate valid for `www.ethnir.x10.mx` was observed
  once with normal verification enabled; the alias is only used after the canonical name
  has already failed a connection twice.
