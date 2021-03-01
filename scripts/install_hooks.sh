#!/usr/bin/env bash
set -ev

ln -s -f '../../scripts/hooks/pre-commit' .git/hooks/pre-commit
