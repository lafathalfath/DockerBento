# Contributing to DockerBento

## Branches

| Branch | Purpose |
|--------|---------|
| `main` | Stable releases only. Tagged with `vX.Y.Z`. |
| `dev` | Integration branch. All features merge here first. |
| `feature/<name>` | New feature or improvement |
| `fix/<name>` | Bug fix |
| `docs/<name>` | Documentation only |

Work on a feature branch, open a pull request against `dev`, and only merge `dev` → `main` when cutting a release.

## Workflow

```bash
# 1. Start from the latest dev
git checkout dev
git pull

# 2. Create a branch
git checkout -b feature/volume-inspect

# 3. Work, commit often
git add src/features/volumes/...
git commit -m "feat(volumes): add inspect detail panel"

# 4. Push and open a PR against dev
git push -u origin feature/volume-inspect
```

## Commit Messages

Follow [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <short summary>

[optional body — what and why, not how]
[optional footer — breaking changes, closes #issue]
```

### Types

| Type | When to use |
|------|------------|
| `feat` | New feature visible to the user |
| `fix` | Bug fix |
| `refactor` | Code change that is neither a fix nor a feature |
| `perf` | Performance improvement |
| `docs` | Documentation only |
| `test` | Adding or fixing tests |
| `chore` | Build scripts, CI, tooling, dependencies |
| `security` | Security hardening |

### Scopes

Use the feature name or layer: `containers`, `images`, `volumes`, `networks`, `settings`, `core`, `shell`, `shared`, `packaging`, `docs`.

### Examples

```
feat(images): add pull progress dialog

fix(containers): prevent double callback fire on socket timeout

security(images): percent-encode container name in run API URL

refactor(core): extract chunked decoding into helper method

docs: add CONTRIBUTING guide and initial commit message template

chore(packaging): add Wayland plugin to AppImage build script
```

### Rules

- Summary line: imperative mood, lowercase after the colon, no period, max 72 characters
- Do not reference line numbers, file names, or PR numbers in the summary — those belong in the body or are derivable from git
- Breaking changes: add `BREAKING CHANGE:` in the footer

## Pull Request Guidelines

- One logical change per PR
- Keep PRs small — easier to review, easier to revert
- Update relevant documentation in the same PR as the code change
- For UI changes: include a short description of what the user sees differently
- For API changes: note the Docker API endpoint and version (e.g., `POST /containers/{id}/rename v1.41`)

## Code Review Checklist

Before marking a PR ready for review:

- [ ] Builds without warnings: `cmake --build build -j$(nproc)`
- [ ] New source files added to `CMakeLists.txt` (SOURCES and HEADERS)
- [ ] User-supplied strings in URL query params use `QUrl::toPercentEncoding()`
- [ ] No `QProcess`, `system()`, or `popen()` calls
- [ ] Dangerous operations have both a `ConfirmDialog` and a visible warning label
- [ ] `Shared::ToggleTable` used instead of raw `QTableWidget`
- [ ] Table stylesheet includes `QTableWidget::item:selected:alternate { background-color: #1565c0; }`
- [ ] Documentation updated if architecture or conventions changed

## Versioning

DockerBento uses [Semantic Versioning](https://semver.org/):

- `MAJOR` — breaking change to the user-visible behavior or configuration format
- `MINOR` — new feature, backwards compatible
- `PATCH` — bug fix, backwards compatible

Version is set in `CMakeLists.txt` (`project(DockerBento VERSION x.y.z)`) and must be updated in `packaging/io.github.dockerbento.DockerBento.appdata.xml` before tagging a release.

```bash
# Tag a release
git tag v1.0.0
git push origin v1.0.0
```
