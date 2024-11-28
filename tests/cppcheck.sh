#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../util/bash-base.sh

echo "[HUSARNET BS] Running cppcheck"

cppcheck_tmp_dir=${base_dir}/build/cppcheck

mkdir -p ${cppcheck_tmp_dir}

# TODO slowly but surely fix the issues and remove suppressions here

cppcheck \
    --error-exitcode=1 \
    --enable=all \
    --suppress=constParameterPointer \
    --suppress=constVariablePointer \
    --suppress=constVariableReference \
    --suppress=cstyleCast \
    --suppress=ctuOneDefinitionRuleViolation \
    --suppress=iterateByValue \
    --suppress=missingInclude \
    --suppress=missingIncludeSystem \
    --suppress=missingOverride \
    --suppress=noExplicitConstructor \
    --suppress=noOperatorEq \
    --suppress=nullPointerRedundantCheck \
    --suppress=passedByValue \
    --suppress=preprocessorErrorDirective \
    --suppress=selfAssignment \
    --suppress=shadowVariable \
    --suppress=toomanyconfigs \
    --suppress=uninitMemberVar \
    --suppress=uninitMemberVarPrivate \
    --suppress=unknownMacro \
    --suppress=unmatchedSuppression \
    --suppress=unreadVariable \
    --suppress=unusedFunction \
    --suppress=unusedPrivateFunction \
    --suppress=useInitializationList \
    --suppress=useStlAlgorithm \
    --suppress=variableScope \
    --inline-suppr \
    --max-configs=10 \
    --config-exclude=esp32 \
    --config-exclude=nlohmann \
    --config-exclude=json \
    --cppcheck-build-dir=${cppcheck_tmp_dir} \
    -I ${base_dir}/core \
    -I ${base_dir}/build/tests/_deps/better_enums-src \
    ${base_dir}/daemon \
    ${base_dir}/core \
    ${tests_base}/unit
