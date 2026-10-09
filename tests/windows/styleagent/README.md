# StyleAgent snapshot and native subscription checks

Build `tst_styleagent`, `tst_portalstyle` and `tst_windowsstyleagent`, then run:

```sh
ctest --test-dir build -C Release -R "^(core\.(styleagent\.unit|portalstyle\.component)|windows\.styleagent)$" --output-on-failure --no-tests=error
```

`core.styleagent.unit` compiles the production StyleAgent notification code and
common `StyleAgentRegistry`. Only platform setup/removal and sampled values are
controlled. Nine added cases cover paired getter values, deduplication, creation
during delivery, deletion of remaining/last agents, owner address reuse,
unregistered owners that are still alive, nested snapshots, pending accent
notifications, and reentry while reading appearance. Assertions do not assume
QSet iteration order. New agents start with Unknown/invalid values in this fixture
so accidental delivery of the old snapshot is observable. The suite requires
20 passes with no skips; this is component coverage, not a native platform test.

`core.portalstyle.component` continues to cover the separate asynchronous Linux
portal state machine. The synchronous registry is not used in that path.

`windows.styleagent` links the real production library. It creates a hidden native
QWindow on the Windows QPA and uses `SendMessageW` to deliver WM_THEMECHANGED,
WM_SYSCOLORCHANGE, WM_DWMCOLORIZATIONCOLORCHANGED and the ImmersiveColorSet form
of WM_SETTINGCHANGE. StyleAgents subscribe through the real application native
filter; no subscription or Windows value reader is replaced. The fixture resets
agent caches to Unknown/invalid to make refresh observable without changing the
user's system settings. It checks paired values, duplicate suppression, last-agent
destruction in direct/nested native callbacks, reinstallation and creation of a
replacement while uninstall is pending. Seven business cases plus Qt Test setup
and teardown must pass, with no skips or expected failures. The existing runner
limits the process to 15 seconds and CTest to 20 seconds; window and agent cleanup
is scoped. CI requires this test in Windows shared/static configurations.

This is native-message/subscription coverage, not physical input, pixel checking,
or verification that changing real OS settings emits all expected notifications.
Tests perform no network operations and need only prepared local dependencies.

## Remaining macOS acceptance (FIX-014)

The common registry preserves the old QSet of private pointers and its iteration
policy. It increments the notification revision before sampling platform values,
captures all owner QPointers before delivery, checks registration before each
callback and stops a superseded snapshot. Each platform still owns initial value
sampling and native subscription lifetime. Windows deferred uninstall and the
macOS distributed-notification observer's allocation/release order are unchanged.
No public header, StyleAgent object layout or exported method is changed by this
extraction; no compatibility shim or ABI-preserving duplicate state is needed.

Native macOS validation is pending; Windows success does not close FIX-014.
On an available local macOS desktop with supported Qt 5 and Qt 6 configurations:

1. Build Core/Widgets/Quick and record exact macOS/SDK/compiler/Qt versions. Run
   the style and portal component suites as component tests separately.
2. Create several real StyleAgents and change theme/accent settings. In each
   signal record both getters, signal counts and notification nesting. Check
   duplicate suppression and consistent theme/accent pairs.
3. During the first notification create an agent, remove another, destroy the
   emitting agent, and repeat with destruction of the final agent. Check the
   distributed observer is released once and a later agent subscribes normally.
4. Recreate an owner at the same address and trigger a nested newer notification.
   Confirm the old snapshot cannot reach a replacement or overwrite newer values.
5. Exercise both AppleInterfaceThemeChangedNotification and
   AppleColorPreferencesChangedNotification; include repeated create/destroy
   cycles and nested notification during final-subscriber teardown. Verify
   subscription removal and absence of callbacks after destruction.

Use bounded waits (for example, five seconds per transition), restore changed
settings and clean up agents after each run. Report unavailable cases explicitly.
No macOS UI CI job is added. The production observer's reentrant release path
must be validated on macOS before the remaining acceptance item is closed.
