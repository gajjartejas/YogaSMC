#!/usr/bin/env python3
"""Compile selected production functions against hardware-free IOKit mocks."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def function(path, signature):
    source = (ROOT / path).read_text()
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

functions = []
for signature in ['UInt16 extractBatteryCycle', 'UInt64 extractBatteryID',
                  'OSData *IdeaVPC::extractBatteryInfo', 'bool IdeaVPC::updateBatteryInfo', 'bool IdeaVPC::toggleConservation',
                  'bool IdeaVPC::toggleRapidCharge', 'bool IdeaVPC::updateBattery(bool',
                  'IOReturn IdeaVPC::setPropertiesResultGated']:
    functions.append(function('YogaSMC/YogaVPC/IdeaVPC.cpp', signature))
for signature in ['IOReturn YogaBaseService::readECName', 'IOReturn YogaBaseService::method_re1b',
                  'IOReturn YogaBaseService::method_recb', 'void YogaBaseService::validateEC']:
    functions.append(function('YogaSMC/YogaBaseService.cpp', signature))
for signature in ['IOReturn YogaSMCUserClient::readECName', 'IOReturn YogaSMCUserClient::readEC(UInt64']:
    functions.append(function('YogaSMC/YogaSMCUserClient.cpp', signature))
common = (ROOT / 'YogaSMC/common.h').read_text()
macros = common[common.index('#define getPropertyBoolean'):common.index('#define setPropertyBoolean')]

with tempfile.TemporaryDirectory(prefix='yogasmc-regressions-') as directory:
    directory = Path(directory)
    source = directory / 'tests.cpp'
    source.write_text((ROOT / 'Tests/mocks.hpp').read_text() + '\n' + macros + '\n' +
                      '\n\n'.join(functions) + '\n' + (ROOT / 'Tests/cases.cpp').read_text())
    for mode in ['release', 'debug']:
        executable = directory / mode
        command = ['clang++', '-std=c++14', '-g', '-fsanitize=address,undefined',
                   '-fno-omit-frame-pointer', '-Werror=return-type']
        if mode == 'debug':
            command.append('-DDEBUG=1')
        subprocess.run(command + [str(source), '-o', str(executable)], check=True)
        print(f'Running {mode} regression cases', flush=True)
        subprocess.run([str(executable)], check=True)
