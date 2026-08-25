# TASK: AUDIT AND FREEZE PROJECT SCOPE

You are auditing an existing C++20 Application Framework repository.

The architecture document provided above is the CURRENT BASELINE.
Treat it as an existing architectural contract, not as a suggestion.

Your job in this task is NOT to redesign the framework.
Your job is to audit the scope and produce a freeze proposal.

==================================================
1. PRIMARY OBJECTIVE
==================================================

Determine whether the current architecture/scope is sufficiently closed
for continued implementation by AI without architecture drift or feature creep.

Identify only:

- scope ambiguity
- contradictory requirements
- duplicated responsibilities
- undefined ownership
- undefined lifecycle behavior
- dependency-layer violations
- API boundary ambiguity
- plugin ABI ambiguity
- testing/release-gate ambiguity
- accidental feature expansion
- roadmap items incorrectly treated as baseline
- requirements that invite AI to invent abstractions

Do NOT implement anything.

Do NOT refactor code.

Do NOT add features.

Do NOT redesign the architecture.

Do NOT introduce new libraries.

Do NOT introduce new abstractions unless explicitly required to resolve
an identified contradiction.

==================================================
2. SOURCE OF TRUTH
==================================================

Priority order:

1. Current repository code
2. Existing tests
3. Current CMake/build configuration
4. The architecture document supplied in this prompt
5. Everything else is NON-AUTHORITATIVE

If documentation and code disagree:

- report the disagreement
- do not silently change either one
- do not choose a new architecture

==================================================
3. AUDIT THE FOLLOWING BOUNDARIES
==================================================

A. Core
B. Services
C. Runtime
D. ModuleManager
E. Built-in Modules
F. Plugin SDK / PluginLoader
G. Application
H. Qt6/QML adapter
I. CMake/package/CI
J. Tests
K. Examples

For each boundary determine:

- What belongs here?
- What explicitly does NOT belong here?
- What dependency direction is allowed?
- Who owns the objects?
- Who controls lifetime?
- What public API is exposed?
- What must never leak across the boundary?

==================================================
4. SCOPE CLOSURE
==================================================

Build three lists:

IN SCOPE
- Features that are part of the framework baseline.

OUT OF SCOPE
- Features explicitly excluded from the framework baseline.

AMBIGUOUS
- Anything where the current document/code does not provide
  enough information to determine whether it belongs.

Do NOT resolve AMBIGUOUS items by guessing.

For every ambiguous item, provide:

- why it is ambiguous
- which existing statement causes ambiguity
- the smallest clarification needed

==================================================
5. FEATURE CREEP DETECTION
==================================================

Look specifically for concepts that could cause an AI developer to
expand the framework unnecessarily.

Examples:

- networking
- HTTP
- database
- SQLite
- persistence
- authentication
- credential management
- audio
- TTS
- UI framework
- dependency injection framework
- reflection
- ECS
- scripting
- coroutine runtime
- task runtime
- distributed runtime
- serialization
- RPC
- metrics/observability platform
- service locator
- event framework expansion
- plugin marketplace/discovery
- hot reload
- configuration framework expansion

Do NOT assume these should be implemented.

Classify each as:

BASELINE
OPTIONAL ADAPTER
APPLICATION RESPONSIBILITY
ROADMAP
OUT OF SCOPE

==================================================
6. ARCHITECTURE DRIFT CHECK
==================================================

Verify these invariants:

- Core never depends on Services, Runtime, Modules or Qt6.
- Services never depend upward on Runtime/Application/UI.
- Runtime owns service instances and ModuleManager.
- Modules do not access ModuleManager internals.
- Qt6 never becomes a dependency of Core/Services/Plugin SDK.
- Plugin runtime boundary is C ABI.
- C++20 language modules are unrelated to runtime plugins.
- Built-in module ownership uses unique ownership.
- Plugin lifetime is tied to dynamic-library lifetime.
- Plugin cannot be unloaded while executable code/data/callbacks remain active.
- RuntimeContext contains non-owning references.
- No global service locator is introduced.
- Operational errors use Result/Error.
- No exception-based redesign is introduced.
- Dependency graph remains acyclic.
- Lifecycle remains explicit and testable.

Report any violation or ambiguity.

==================================================
7. LIFECYCLE AUDIT
==================================================

Audit:

Discovered
Loaded
Initialized
Started
Running
Stopping
Stopped
Unloaded

Check:

- valid transitions
- invalid transitions
- restart behavior
- dependency ordering
- rollback
- partial failure
- stop failure
- plugin unload behavior
- repeated initialize/start/stop
- destruction after failure

Do not invent new lifecycle states unless required to explain
an existing contradiction.

==================================================
8. OWNERSHIP / LIFETIME AUDIT
==================================================

Produce an ownership table:

Object | Owner | Lifetime | Destroyed by | Cross-boundary rule

At minimum include:

- services
- RuntimeContext
- ModuleManager
- built-in module
- plugin module
- LoadedPlugin
- dynamic library handle
- SubscriptionToken
- scheduler task
- callbacks

Pay particular attention to:

stop
-> cancel task
-> unsubscribe
-> destroy module
-> unload library

If this ordering is insufficiently specified, report it.
Do not redesign it automatically.

==================================================
9. PUBLIC API AUDIT
==================================================

Identify APIs that appear to be:

- framework contract
- implementation detail
- accidental public API
- insufficiently specified

Do not propose a large API redesign.

The goal is to minimize the public surface.

==================================================
10. CMAKE / PACKAGE SCOPE
==================================================

Verify:

- exported targets
- dependency visibility
- optional Qt6 boundary
- BUILD_* options
- package consumer expectations
- test/build configuration

Identify anything that could cause an AI to accidentally make Qt6,
SQLite, networking or other application dependencies mandatory.

==================================================
11. TEST CONTRACT AUDIT
==================================================

Determine whether every important architectural invariant has a test
or an explicit reason why it does not.

Focus on:

- lifecycle
- dependency graph
- rollback
- plugin ABI
- plugin unload safety
- ownership
- callback lifetime
- package consumption
- Qt6 isolation
- Application failure mapping

Do NOT create tests in this task.

Only identify missing contract coverage.

==================================================
12. FINAL FREEZE PROPOSAL
==================================================

Produce the result in exactly this structure:

# 1. AUDIT RESULT

Status:
- GREEN = scope is sufficiently closed
- YELLOW = clarification required
- RED = architectural contradiction exists

# 2. NON-NEGOTIABLE ARCHITECTURAL INVARIANTS

List concise rules that AI must never violate.

# 3. IN SCOPE

List the final baseline capabilities.

# 4. OUT OF SCOPE

List explicit exclusions.

# 5. APPLICATION RESPONSIBILITIES

List things applications must implement themselves.

# 6. AMBIGUITIES

For every ambiguity:
- Issue
- Evidence
- Minimal clarification required

# 7. ARCHITECTURAL CONTRADICTIONS

Only actual contradictions.
Do not manufacture hypothetical ones.

# 8. OWNERSHIP / LIFETIME CONTRACT

Provide the ownership table.

# 9. LIFECYCLE CONTRACT

Provide valid transitions and failure rules.

# 10. TEST GAPS

Only missing contract tests that materially affect architecture safety.

# 11. PROPOSED FREEZE PATCH

Produce a MINIMAL textual patch to the architecture contract.

Do not rewrite the entire architecture document.

The patch must only:
- clarify
- close ambiguity
- explicitly exclude scope
- define missing invariants

It must NOT:
- add product features
- redesign architecture
- introduce dependencies
- introduce abstractions for hypothetical future use

# 12. AI IMPLEMENTATION RULES

End with a short ruleset that can be pasted into AGENTS.md,
CLAUDE.md, or another AI coding instruction file.

==================================================
13. CHANGE CONTROL
==================================================

After this audit, the architecture is considered FROZEN.

Any future change involving:

- new framework subsystem
- new dependency
- new public API
- ownership model
- lifecycle state
- plugin ABI
- dependency direction
- threading model
- error model
- Qt6 boundary
- packaging model

requires explicit user approval.

AI MUST NOT make such changes autonomously.

If a requested feature conflicts with the frozen contract:

STOP.

Explain the conflict.

Ask for approval.

Do not implement a workaround that silently changes architecture.

==================================================
14. IMPORTANT
==================================================

This is a SCOPE AUDIT, not an implementation task.

Do not modify files.

Do not write code.

Do not refactor.

Do not "improve" architecture.

Do not add future-proof abstractions.

Do not optimize hypothetical use cases.

Do not turn roadmap items into framework features.

The desired outcome is a SMALLER, CLEARER and MORE CLOSED framework,
not a more sophisticated framework.