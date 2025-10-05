# Curious C++ STYLE

> Goal: instantly read **access**, **const-ness**, and **indirection/ownership** from names — while keeping things pleasant to look at.

---

## 1) Access Levels & Casing (Fields & Methods)

### Data members (fields)

* **Public** → `_snake_case`
* **Protected** → `camelCase`
* **Private** → `_camelCase` (leading underscore + lowerCamel)

**Rationale:** Public fields are rare and visually distinct; protected feels natural for subclassing; private is compact and readable without trailing underscores.

### Functions / Methods

* **Public** → `snake_case`
* **Protected** → `camelCase`
* **Private** → `CamelCase` (PascalCase)

**Examples**

```cpp
class Session {
public:
  // Public API
  void connect_to_server();               // snake_case
  bool is_active() const;                 // snake_case
  int  _id;                               // public field (rare): _snake_case
  static constexpr int _k_buffer_size = 4096; // public const: _k + snake_case

protected:
  // Protected API & fields
  void refreshToken();                    // camelCase
  static std::shared_ptr<Session> makeDefaultSptr();
  int maxConnections;                     // camelCase
  static constexpr int kDefaultTimeout = 1500; // k + camelCase

private:
  // Private API & fields
  void Reconnect();                       // CamelCase
  static void LoadKeys();                 // CamelCase

  std::unique_ptr<Connection> _connUptr;  // _camelCase + Uptr suffix
  std::shared_ptr<User>       _ownerSptr; // _camelCase + Sptr suffix
  std::weak_ptr<Session>      _parentWptr;// _camelCase + Wptr suffix
  char*                       _bufPtr;    // _camelCase + Ptr suffix
  const Config&               _cfgRef;    // _camelCase + Ref suffix

  static constexpr int _kMaxRetries = 3;  // _k + CamelCase
};
```

---

## 2) Const / Immutable

* **Prefix** with `k` (protected) or `_k` (public/private), then follow that accessor’s **casing**.

  * Public const: `_k_buffer_size` (snake\_case)
  * Protected const: `kDefaultTimeout` (camelCase)
  * Private const: `_kMaxRetries` (CamelCase or camelCase depending on placement; prefer CamelCase for private methods/fields context)

---

## 3) Indirection & Ownership (Suffixes)

Append these **suffixes** to the base name:

* Raw pointer → `Ptr`   → `*_ptr` / `*Ptr` (depends on accessor’s base casing)
* `std::unique_ptr` → `Uptr`
* `std::shared_ptr` → `Sptr`
* `std::weak_ptr` → `Wptr`
* Owning value of a non-RAII resource → `Own`
* Reference → `Ref`

**Examples**

```cpp
std::unique_ptr<Connection> _connUptr; // private
std::shared_ptr<User>       ownerSptr; // protected
std::weak_ptr<Session>      _parentWptr; // private
char*                       _bufPtr;   // private raw pointer
const Config&               cfgRef;    // protected reference
```

---

## 4) Statics, Globals, Thread-Local

* **Statics** follow the accessor’s naming, optionally prefixed with `s_` if you want extra punch in large files: `s_kDefaultTimeout`, `_s_connUptr`.
* **Globals**: `g_` + `snake_case` (keep them rare): `g_shutdown`.
* **thread\_local**: `t_` + follow accessor family: `t_depth`, `_t_depthLocal`.

---

## 5) Namespaces, Types, Files

* **Types** (classes, structs, enum classes): `PascalCase` → `NetworkMessage`, `UserRecord`.
* **Enum class values**: `PascalCase` → `Status::Success`.
* **Namespaces**: `lowercase_with_underscores` → `curious::securityd`.
* **Files**: `lowercase_with_underscores.[h|hpp|cc|cpp]` → `security_daemon.cpp`.

---

## 6) Parameters & Locals

* Keep **clean**, don’t encode `const` (the type shows it).
* Out params: add `Out` suffix (`resultOut`, `dataOut`).
* In-out params: `InOut` suffix (`stateInOut`). Prefer return values / structs over output params when possible.

---

## 7) Quick Matrix (Fields)

| Access        | Pattern       | Examples                                   |
| ------------- | ------------- | ------------------------------------------ |
| **Public**    | `_snake_case` | `_id`, `_k_buffer_size`, `_buf_ptr`        |
| **Protected** | `camelCase`   | `maxConnections`, `ownerSptr`, `cfgRef`    |
| **Private**   | `_camelCase`  | `_connUptr`, `_parentWptr`, `_kMaxRetries` |

**Methods**: Public `snake_case`, Protected `camelCase`, Private `CamelCase`.

---

## 8) FAQ

* **Why leading underscores on public fields?** To make them unmistakable (and rare). This also avoids collisions with method names in fluent APIs.
* **Why private methods in CamelCase?** They visually separate internal machinery from public/protected flows.
* **Do I have to use `s_` for statics?** Optional. Use it in large files where visual grepping helps.

---

# Tooling

Naming is enforced via **clang-tidy**; layout/format via **clang-format**.

## `.clang-format`

Use this to get consistent indentation, wrapping, braces, includes, etc. (It cannot enforce naming — that’s clang-tidy.)

```yaml
# .clang-format
BasedOnStyle: LLVM
IndentWidth: 4
TabWidth: 4
UseTab: Never
ColumnLimit: 100
AllowShortFunctionsOnASingleLine: Empty
BreakBeforeBraces: Allman
SpacesInParentheses: false
SpaceAfterCStyleCast: true
PointerAlignment: Left
DerivePointerAlignment: false
AllowShortIfStatementsOnASingleLine: Never
NamespaceIndentation: All
IndentCaseLabels: true
SortIncludes: true
IncludeBlocks: Regroup
IncludeCategories:
  - Regex: '^<[^/]+>$'
    Priority: 1
  - Regex: '^<.*/>'
    Priority: 2
  - Regex: '^[^"]+\.h(pp)?$'
    Priority: 3
  - Regex: '^"'
    Priority: 4
QualifierAlignment: Left
AlignConsecutiveAssignments: Consecutive
AlignConsecutiveDeclarations: Consecutive
ReflowComments: true
```

## `.clang-tidy`

This enforces the **naming rules**. (Clang-tidy 16+ recommended.)

```yaml
# .clang-tidy
Checks: >-
  -*,
  readability-identifier-naming,
  modernize-*,
  performance-*,
  cppcoreguidelines-avoid-goto,
  cppcoreguidelines-non-private-member-variables-in-classes,
  cppcoreguidelines-pro-bounds-array-to-pointer-decay,
  cppcoreguidelines-pro-type-const-cast,
  cppcoreguidelines-pro-type-cstyle-cast,
  cppcoreguidelines-pro-type-reinterpret-cast,
  hicpp-use-override,
  readability-braces-around-statements,
  readability-implicit-bool-conversion,
  readability-magic-numbers

WarningsAsErrors: ''

CheckOptions:
  # ---- Methods ----
  - { key: readability-identifier-naming.PublicMethodCase,    value: lower_case }
  - { key: readability-identifier-naming.ProtectedMethodCase, value: camelBack }
  - { key: readability-identifier-naming.PrivateMethodCase,   value: CamelCase }

  # ---- Fields ----
  - { key: readability-identifier-naming.PublicMemberCase,    value: lower_case }
  - { key: readability-identifier-naming.PublicMemberPrefix,  value: _ }

  - { key: readability-identifier-naming.ProtectedMemberCase,   value: camelBack }
  - { key: readability-identifier-naming.ProtectedMemberPrefix, value: '' }

  - { key: readability-identifier-naming.PrivateMemberCase,   value: camelBack }
  - { key: readability-identifier-naming.PrivateMemberPrefix, value: _ }

  # ---- Constants (per access) ----
  - { key: readability-identifier-naming.PublicConstantCase,    value: lower_case }
  - { key: readability-identifier-naming.PublicConstantPrefix,  value: _k }

  - { key: readability-identifier-naming.ProtectedConstantCase,   value: camelBack }
  - { key: readability-identifier-naming.ProtectedConstantPrefix, value: k }

  - { key: readability-identifier-naming.PrivateConstantCase,   value: CamelCase }
  - { key: readability-identifier-naming.PrivateConstantPrefix, value: _k }

  # ---- Types ----
  - { key: readability-identifier-naming.TypeCase, value: PascalCase }
  - { key: readability-identifier-naming.EnumCase, value: PascalCase }
  - { key: readability-identifier-naming.EnumConstantCase, value: PascalCase }

  # ---- Namespaces & Files ----
  - { key: readability-identifier-naming.NamespaceCase, value: lower_case }
  - { key: readability-identifier-naming.GlobalFunctionCase, value: lower_case }

  # NOTE: Enforcing pointer/ownership suffixes (Ptr/Uptr/Sptr/Wptr/Own/Ref)
  # is not natively supported by readability-identifier-naming per-type.
  # If desired, add a custom script or pre-commit regex check.
```

### Optional: pre-commit regex for pointer/ownership suffixes

You can add a lightweight script to flag names that don’t end with `Ptr/Uptr/Sptr/Wptr/Own/Ref` when the type matches.

```bash
# hooks/pre-commit (excerpt idea)
# grep for common pointer/owner patterns lacking the suffix; tune for your codebase

git diff --cached -U0 | \
  grep '^\+[^+]' | \
  grep -E '(std::(unique_ptr|shared_ptr|weak_ptr)|\*|&)[[:space:]]+([A-Za-z_][A-Za-z0-9_]*)[[:space:]]*(=|;|,)' | \
  grep -vE '(Uptr|Sptr|Wptr|Ptr|Ref|Own)[[:space:]]*(=|;|,)' && {
    echo "[naming] Some pointer/reference/owner variables are missing required suffix (Ptr/Uptr/Sptr/Wptr/Ref/Own)";
    exit 1;
}
```

---

## 9) Usage Tips

* Put these files at repo root: `.clang-format`, `.clang-tidy`.
* Enable **Clang-Tidy** in your IDE (CLion, VSCode with clangd, Visual Studio LLVM toolset). Turn on “Fixits” for easy renames.
* For third-party code, exclude via `-header-filter` or clang-tidy config `HeaderFilterRegex`.

---

Happy building! This gives you clean visuals at call sites, strong access signals, and portable tooling to keep it consistent across your Curious repos.
