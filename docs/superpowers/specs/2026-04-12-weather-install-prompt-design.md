# Weather Install Prompt Design

## Summary

Add a post-install user prompt for the DDE Shell weather plugin package. After package installation or upgrade completes, if a graphical user session is currently active, the package should launch a small DTK-based helper inside that user session. The helper shows a friendly dialog explaining that the weather plugin display and updates depend on reloading the taskbar. If the user agrees, the helper restarts the taskbar by running `systemctl --user restart dde-shell@DDE.service`.

If there is no active graphical session, the package must skip the prompt silently and must not block package installation.

## Goals

- Explain to users why the plugin may not appear immediately after installation.
- Give users a one-click DTK-native way to restart the taskbar.
- Cover both first-time installation and package upgrades with user-friendly wording.
- Ensure all new user-visible install-time strings are internationalized.
- Never block or fail package installation because of dialog or session-launch issues.

## Non-Goals

- Persisting reminders for future logins when no session is active.
- Forcing an automatic taskbar restart without user consent.
- Building a multi-step install wizard.
- Retrying or monitoring the taskbar restart after the command is issued.

## User Experience

### Installation Case

When the package is newly installed and an active graphical user session is found, show a DTK dialog with wording equivalent to:

Title: `天气插件提示`

Body:
`天气插件已安装完成。天气信息显示和自动更新依赖任务栏重新加载。现在重启任务栏后，天气插件会显示在任务栏中，并开始更新天气信息。`

Buttons:
- Secondary: `稍后再说`
- Primary: `立即重启`

### Upgrade Case

When the package is upgraded and an active graphical user session is found, show the same style dialog with wording equivalent to:

Title: `天气插件提示`

Body:
`天气插件已更新完成。为了让天气插件继续正常显示并更新天气信息，需要重新加载任务栏。现在重启后，任务栏会很快自动恢复。`

Buttons:
- Secondary: `稍后再说`
- Primary: `立即重启`

### Interaction Rules

- The dialog is shown once per install or upgrade transaction when a graphical session is available.
- Choosing `稍后再说` closes the dialog and does nothing else.
- Choosing `立即重启` launches `systemctl --user restart dde-shell@DDE.service` from the user session and then exits.
- If restart command launch fails, the helper may log diagnostics but must not show follow-up error dialogs.

### Internationalization

- All new helper strings must be translatable and must not be hardcoded as Chinese-only runtime text.
- The helper must use the project's existing Qt translation flow so that title text, body text, and button labels can be localized consistently with the rest of the plugin.
- `postinst` should pass only mode information such as `install` or `upgrade`; localized user-visible wording belongs in the helper, not in the shell script.
- The initial Chinese wording in this document is the source intent, not an instruction to bypass translation support.

## Packaging Design

### Maintainer Script Hook

Add a `debian/postinst` script and handle the `configure` action only.

- `postinst configure` with an empty second argument is treated as first install.
- `postinst configure <old-version>` is treated as upgrade.
- All other actions should exit successfully without side effects.

The script must be defensive and non-blocking. If any prerequisite for showing the dialog is missing, it exits `0`.

### Session Detection

`postinst` must locate an active local graphical session before attempting to show the dialog.

Selection strategy:
- Enumerate sessions with `loginctl list-sessions`.
- Inspect session properties with `loginctl show-session`.
- Prefer a session that is:
  - `Active=yes`
  - `State=active`
  - `Remote=no`
  - `Type=wayland` or `Type=x11`
- Extract at minimum:
  - session id
  - user name
  - uid
  - display value when available

If no qualifying session exists, skip silently.

### Launching into the User Session

`postinst` must not try to show the dialog as root. It should switch into the detected desktop user context and launch the helper asynchronously.

Environment to provide:
- `XDG_RUNTIME_DIR=/run/user/<uid>`
- `DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/<uid>/bus`
- `DISPLAY=<display>` when present
- `WAYLAND_DISPLAY=<name>` when present from the session or runtime environment, if discoverable

Launch style:
- Use `runuser -u <user> -- env ... <helper> --mode install|upgrade`
- Background the helper so `dpkg` is not held open waiting for user input
- Ignore helper launch failure after optional logging

The package installation result must not depend on the helper succeeding.

## Helper Design

### Binary

Add a small Qt/DTK executable dedicated to installation prompting. Proposed name:

- `ds-weather-install-prompt`

Install it to a package-owned executable path suitable for maintainer-script invocation, for example under `/usr/libexec/dde-shell-weather-plugin/`.

### Implementation Responsibilities

The helper is responsible for:
- parsing `--mode=install|upgrade`
- creating a DTK application object
- showing a DTK-styled modal confirmation dialog with the correct localized text
- sourcing all user-visible text through Qt translation APIs
- starting `systemctl --user restart dde-shell@DDE.service` only after explicit confirmation
- exiting immediately after user dismissal or command start

The helper is not responsible for:
- discovering sessions
- retrying restart
- persisting state
- running as root

### UI Technology

Use DTK widgets already available in the package build environment. The dialog should follow the existing desktop style instead of introducing custom QML or complex assets. A widget-based dialog is sufficient and preferred for this helper.

## Failure Handling

The design deliberately treats this feature as best-effort.

Expected non-fatal failure cases:
- no active graphical session
- no usable user bus
- missing display variables
- `runuser` launch failure
- helper startup failure
- user closes the dialog
- restart command launch failure

In every case above:
- package installation must still succeed
- no automatic restart is attempted without user confirmation
- no repeated prompts are scheduled by this feature

## Testing Strategy

### Packaging Logic

Validate that `debian/postinst`:
- distinguishes install vs upgrade correctly from arguments
- exits cleanly on non-`configure` actions
- exits cleanly when no active graphical session is present
- launches the helper with the expected mode when a session is found

Where direct integration testing is hard, keep the shell logic factored so core decision points can be exercised with command stubs or environment overrides.

### Helper Logic

Validate that the helper:
- accepts only supported mode values
- selects the correct strings for install and upgrade
- resolves all user-visible strings through the translation path used by the executable
- triggers `systemctl --user restart dde-shell@DDE.service` only on positive confirmation

### Manual Verification

Manual checks should cover:
- install while logged into DDE: dialog appears with install wording
- upgrade while logged into DDE: dialog appears with upgrade wording
- clicking `稍后再说`: no restart occurs
- clicking `立即重启`: taskbar restarts and comes back
- install or upgrade without graphical login: no prompt and package succeeds

## File-Level Plan

- Add `debian/postinst` for install-time session detection and helper launch.
- Add a new C++ source pair for the DTK helper.
- Update `CMakeLists.txt` to build and install the helper executable.
- Update Debian packaging metadata if additional installation directives are needed.
- Add tests for helper text-selection or command-trigger behavior where practical.

## Risks and Mitigations

Risk: user session discovery is brittle across desktop setups.
Mitigation: keep detection conservative, skip silently when uncertain, and target only active local graphical sessions.

Risk: missing runtime environment prevents a DTK app from opening.
Mitigation: pass only minimal required session variables and treat failures as non-fatal.

Risk: package scripts become harder to maintain.
Mitigation: keep `postinst` small, linear, and best-effort; move UI behavior into the helper binary.

## Decision Summary

- Use `debian/postinst` at `configure` time.
- Show a DTK-native prompt only when an active graphical user session exists.
- Differentiate install and upgrade wording.
- Restart the taskbar only after explicit user confirmation.
- Skip silently when the environment is unsuitable.
