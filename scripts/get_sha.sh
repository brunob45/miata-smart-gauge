#!/usr/bin/env sh

version=$(git rev-parse --short HEAD)

if ! $(git diff --quiet); then
    #workspace is dirty
    version=${version}*;
fi

mkdir -p lib/git_sha/

printf "
#ifndef GIT_SHA
#define GIT_SHA \"${version}\"
#endif
">lib/git_sha/git_sha.h