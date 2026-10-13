#!/usr/bin/env python3
"""Run the shader contract regression; optionally check OF headers and macOS GPU.

From this addon: python3 tests/shader_editor_regression.py --syntax --gpu
Requires sibling ofxOceanode/ofxImGui and the parent openFrameworks tree for
--syntax. --gpu requires macOS desktop GPU access. Neither launches a host app.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--syntax', action='store_true')
    parser.add_argument('--gpu', action='store_true')
    args = parser.parse_args()
    cxx = os.environ.get('CXX', 'clang++')
    with tempfile.TemporaryDirectory(prefix='shader-editor-test-') as temp:
        temp = Path(temp)
        binary = temp / 'contract'
        subprocess.run([cxx, '-std=c++17', '-Wall', '-Wextra', '-I' + str(ROOT / 'src'),
                        str(ROOT / 'tests/shader_source_regression.cpp'), '-o', str(binary)], check=True)
        subprocess.run([str(binary), str(ROOT)], check=True)
        save_binary = temp / 'save'
        subprocess.run([cxx, '-std=c++17', '-Wall', '-Wextra', '-I' + str(ROOT / 'src'),
                        str(ROOT / 'tests/shader_save_regression.cpp'), '-o', str(save_binary)], check=True)
        subprocess.run([str(save_binary)], check=True)
        update_binary = temp / 'effects-update'
        subprocess.run([cxx, '-std=c++17', '-Wall', '-Wextra', '-I' + str(ROOT / 'src'),
                        str(ROOT / 'tests/effects_library_regression.cpp'), '-o', str(update_binary)], check=True)
        subprocess.run([str(update_binary), str(ROOT)], check=True)
        if args.syntax:
            of = ROOT.parent.parent
            includes = sorted({p.parent for p in (of / 'libs/openFrameworks').rglob('*.h')})
            includes += [p for p in (of / 'libs').glob('*/include') if p.is_dir()]
            includes += [of / 'libs/freetype/include/freetype', of / 'libs/cairo/include/cairo']
            for addon in ('ofxOceanode', 'ofxOceanodeTextures', 'ofxImGui', 'ofxSyphon'):
                base = of / 'addons' / addon
                includes += [base / 'src', *sorted({p.parent for p in (base / 'src').rglob('*.h')})]
                includes += sorted({p.parent for p in (base / 'libs').rglob('*.h')})
            source = temp / 'shader_editor.cpp'
            source.write_text('#include "shaderEditor.h"\n#include "ofxOceanodeTextures.h"\n')
            subprocess.run([cxx, '-std=c++17', '-fsyntax-only', '-w', '-DOF_NO_FMOD',
                            '-DGL_SILENCE_DEPRECATION', *['-I' + str(p) for p in dict.fromkeys(includes)],
                            str(source), str(of / 'addons/ofxOceanode/src/Controls/ofxOceanodeInspectorController.cpp'),
                            str(of / 'addons/ofxOceanode/src/ofxOceanode.cpp')], check=True)
            print('PASS Shader editor, Textures registration, Inspector and Config menu syntax against local OF/addon headers', flush=True)
        if args.gpu:
            gpu = temp / 'gpu'
            subprocess.run([cxx, '-std=c++17', '-DGL_SILENCE_DEPRECATION',
                            '-I' + str(ROOT / 'src'), '-I' + str(ROOT / 'src/shaders'),
                            str(ROOT / 'tests/shader_gl_regression.cpp'), '-framework', 'OpenGL',
                            '-o', str(gpu)], check=True)
            subprocess.run([str(gpu), str(ROOT)], check=True)


if __name__ == '__main__':
    main()
