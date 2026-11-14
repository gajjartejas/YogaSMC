# Local validation

Date: November 14, 2026. Development driver version: 2.0.1. Branch: `codex/ideapad-ec-robustness`.

| Check | Result |
| --- | --- |
| Production-function regression tests, release mode | PASS with AddressSanitizer and UndefinedBehaviorSanitizer |
| Production-function regression tests, debug mode | PASS with AddressSanitizer and UndefinedBehaviorSanitizer |
| YogaSMC Release, x86_64 | PASS, Xcode 26.6, signing disabled |
| YogaSMC Debug, x86_64 | PASS, Xcode 26.6, signing disabled |
| YogaSMCAlter Debug, x86_64 | PASS, Xcode 26.6, signing disabled |
| Whitespace/error-marker diff check | PASS |
| Loaded kext / firmware changes / boot test | Not performed |
| Hardware-free regression workflow | Configured for push and pull-request events; see repository Actions for remote run results |

The tests compile actual selected function bodies from the working source against mocked IOKit/ACPI objects. They include zero/short/valid/wrong-type battery data, decoded temperature, all conservation/rapid-charge state combinations, ignored writes, read/write failures, manual conservation requests while the automatic SMC lock is set, unsupported/invalid charging requests, duplicate no-op requests, multi-key first-error behaviour, four-byte nonterminated EC names, invalid names, null providers, end-of-EC and overflowed-offset reads, failed/mismatched bulk reads, capability changes, and invalid property types.

Full builds cover the real kext headers, class overrides and both normal/ALTER variants. The Debug target runs the project's configured static analyzer. Remaining build messages concern the pre-existing legacy deployment target/toolchain flags and the local CoreSimulator version mismatch. No firmware, boot, timing, signing, IOKit runtime, or sleep/wake compatibility claim follows from these checks.

Build outputs and local log copies are under ignored `build/`; dependency revisions/hashes and commands are in [LOCAL_CHANGES.md](LOCAL_CHANGES.md).

The 2.0.1 conservation correction preserves the automatic SMC lock while allowing the legacy manual control path. Tests use the actual default lock state (true), verify manual success and repeated-request idempotence, and check that the lock is not cleared. Local build verification covers the 2.0.1 source; no EFI installation or live firmware write was performed for these checks.
