#!/bin/bash
# Backwards-compatible entry point used by the installers.
exec bash "$(dirname "$0")/glsl-compile.sh" "$@"
