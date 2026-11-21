#!/bin/bash
# Stop-Hook: Änderungen am Ende eines Claude-Turns committen und nach GitHub pushen.
# secrets.yaml, .venv und .esphome sind per .gitignore ausgeschlossen.
cd "${CLAUDE_PROJECT_DIR:-$(dirname "$0")/../..}" || exit 0
git rev-parse --git-dir >/dev/null 2>&1 || exit 0

git add -A
if ! git diff --cached --quiet; then
  files=$(git diff --cached --name-only | head -5 | paste -sd ',' | sed 's/,/, /g')
  git commit -q -m "Auto-Commit: $files" \
    -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>" || exit 0
fi

# Push nur, wenn lokale Commits fehlen auf origin
if [ -n "$(git log '@{u}..' --oneline 2>/dev/null)" ]; then
  if git push -q 2>/tmp/autopush-err.$$; then
    echo '{"systemMessage": "Änderungen nach GitHub gepusht"}'
  else
    echo "{\"systemMessage\": \"Auto-Push fehlgeschlagen: $(tr -d '\"\n' < /tmp/autopush-err.$$ | head -c 200)\"}"
  fi
  rm -f /tmp/autopush-err.$$
fi
exit 0
