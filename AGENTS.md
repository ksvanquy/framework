# AI DEVELOPMENT CONTRACT

## Architecture Status

The framework architecture is FROZEN.

The architecture document is a contract.

Do not redesign it during normal feature implementation.

## Default Behavior

For every task:

1. Identify the affected layer.
2. Identify the existing contract.
3. Implement the smallest change necessary.
4. Do not modify unrelated layers.
5. Do not introduce new abstractions without explicit approval.
6. Do not add dependencies without explicit approval.
7. Do not expand framework scope.

## Forbidden Autonomous Changes

AI must NOT autonomously introduce:

- networking
- HTTP
- SQLite
- persistence implementations
- authentication
- credential management
- audio
- TTS
- reflection
- ECS
- DI framework
- service locator
- serialization framework
- RPC
- scripting
- hot reload
- C++20 Modules
- new plugin mechanisms
- new lifecycle states
- new framework services
- new public API
- new third-party dependency

unless explicitly requested and approved.

## Scope Rule

If a requested change is outside the current framework scope:

STOP.

Do not implement it inside the framework.

Determine whether it belongs to:

- application
- optional adapter
- plugin
- roadmap
- out of scope

Ask the user if classification is unclear.

## Architecture Rule

Dependency direction must remain downward.

Core -> Services -> Runtime -> Modules/Plugins -> Application/UI

Never introduce an upward dependency.

## Ownership Rule

Every new object must have an explicit owner.

Do not introduce shared ownership merely for convenience.

Prefer existing ownership patterns.

## Public API Rule

Minimize public API.

Before adding a public type/function/interface, verify that the
existing contract cannot solve the problem.

## Change Escalation

STOP and request approval if the implementation requires changing:

- public API
- ownership
- lifetime
- lifecycle
- dependency direction
- plugin ABI
- threading model
- error model
- CMake package boundary
- Qt6 boundary
- framework scope

## Implementation Principle

Prefer:

SMALLEST CHANGE
over
GENERAL ABSTRACTION

Prefer:

EXISTING CONTRACT
over
NEW DESIGN

Prefer:

APPLICATION CODE
over
FRAMEWORK CODE

when a feature is application-specific.

## Completion Rule

A task is complete when its requested behavior works and its existing
contract/tests pass.

Do not implement additional improvements "while you are here".