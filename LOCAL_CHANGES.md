# IdeaPad and EC robustness changes

The robustness changes in this fork start at upstream `299907b359c4d323e8c45ecc91e88c2a87b6082d`.

## Behaviour changes

- Reject undersized battery cycle/ID/GSBI buffers before reading them. Decode multi-byte data with `memcpy`; preserve full five-digit cycle counts.
- Treat a four-byte EC field name as a bounded ACPI NameSeg. Copy it to a terminated local buffer, validate characters, and use the base service's checked EC lookup.
- Validate direct EC read ranges without integer-overflow-prone addition. Clear failed bulk-read output pointers and avoid copying uninitialized data after an ACPI error. Check the provider before dereferencing it during notification registration.
- Fix the property macros so invalid types skip the enclosing property loop rather than continuing with null values. These macros are intended only for loop-based property handlers.
- Read `GBMD` after conservation/rapid-charge writes. Publish the observed firmware state instead of an assumed toggle; rapid-charge publication no longer uses the conservation-mode value.
- Return errors for **IdeaVPC charging requests**: invalid types, unsupported rapid charging, failed reads/writes, unconfirmed changes. Multiple properties are best-effort rather than atomic: each is processed and the first error is returned. An error after a write can mean the change was not yet reflected in firmware; it does not guarantee that nothing changed. Debug-only raw SBMC requests retain their existing interface.
- Preserve legacy handling and result semantics for other properties and VPC variants. This is not a rewrite of every driver's control API.
- Keep `Basic`, `RO` and `RW` meanings, reset stale capability bits, and expose `EC Read Byte Method`, `EC Read Buffer Method`, `EC Write Byte Method` and `EC Capability Reason`. A missing EC reports `Unavailable`. These properties describe ACPI method availability, not a hardware test or charging-control permission level.

This does not enable ECRW ACPI helpers, install a driver, enable raw writes, change EFI/NVRAM, or claim to fix sleep or charger-related trackpad lag.

## Conservation-mode correction — 2.0.1 (November 14, 2026)

The initial 2.0.0 hardening mistakenly rejected manual conservation changes while `conservationModeLock` was true. That upstream flag defaults to true and guards automatic SMC writes; the legacy pane's explicit manual path did not use it. Manual requests now retain the read-back/error handling without applying or clearing that automatic-control lock. SMC callback gating and raw EC locking are unchanged. Regression mocks now use the production default (locked) and verify manual success, idempotence and preservation of the automatic lock. The legacy YogaSMCPane code is untouched.

## Regression checks

```sh
python3 Tests/run_regressions.py
```

Requires Python 3 and clang++. The runner extracts the actual changed production functions and compiles them against hardware-free IOKit mocks in both release and debug modes, with AddressSanitizer and UndefinedBehaviorSanitizer enabled. It tests buffer lengths and types, charging states and failed/ignored writes, charging request results and no-ops, EC names/ranges, capability revalidation and invalid-property control flow. It does not load a kext or invoke hardware. A separate workflow runs it on pushes and pull requests without downloading the kernel SDKs.

Mock tests do not validate IOKit ABI/lifetime/concurrency, full ACPI behaviour, firmware timing, or boot compatibility. Actual kext compilation supplements these tests; hardware trials require a separate test EFI and rollback copy.

## Local build dependencies

Official dependency inputs used for this build (downloaded into ignored directories):

| Input | Revision / SHA-256 |
| --- | --- |
| MacKernelSDK | `3f750085caa17ec3a7880f11c11bf4f48cd6a164` |
| Lilu 1.7.2 DEBUG ZIP | `e95df95d82e8b151047abf359ecd9c853755c59293e19dfcc3907da8fbd8cf89` |
| VirtualSMC 1.3.8 DEBUG ZIP | `d1a13a4ad5564f97d4fa4cb584eaa63cb4fbe280c6997100133789dd4bdac200` |

Sources: [MacKernelSDK](https://github.com/acidanthera/MacKernelSDK), [Lilu 1.7.2](https://github.com/acidanthera/Lilu/releases/tag/1.7.2), [VirtualSMC 1.3.8](https://github.com/acidanthera/VirtualSMC/releases/tag/1.3.8).

The debug bundles provide build headers; they are not installed. VirtualSMC's release ZIP stores its bundle under `Kexts/VirtualSMC.kext`: place that bundle at the checkout root alongside `Lilu.kext`. No remote bootstrap script is evaluated.

Build commands with Xcode 26.6:

```sh
xcodebuild -project YogaSMC.xcodeproj -target YogaSMC -configuration Release CODE_SIGNING_ALLOWED=NO build
xcodebuild -project YogaSMC.xcodeproj -target YogaSMC -target YogaSMCAlter -configuration Debug CODE_SIGNING_ALLOWED=NO build
```

Outputs stay in ignored `build/`. They are development artifacts, not a published or hardware-validated release. Existing Xcode warnings include the legacy 10.9 deployment target and a local CoreSimulator version mismatch. Build results are recorded in `VALIDATION.md`.
