"""Compatibility for the pinned ArduinoFake/FakeIt on Apple Clang21.

That dependency uses a formerly accepted dependent-template spelling. Keep the
diagnostic visible, but nonfatal; do not edit a downloaded dependency or weaken
warnings in the STM32 production build.
"""
import sys
Import('env')
if sys.platform == 'darwin':
    env.Append(CCFLAGS=['-Wno-error=missing-template-arg-list-after-template-kw'])
