"""Compile the actual recovered grid loop against hardware mocks and test USB feedback."""
from pathlib import Path
import shutil
import subprocess
import unittest
ROOT=Path(__file__).resolve().parent
class ControllerTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which('clang++'),'Native controller checks require clang++')
    def test_feedback_and_preserved_keyboard(self):
        output=ROOT/'out/controller-tests/feedback-test';output.parent.mkdir(parents=True,exist_ok=True)
        subprocess.run(['clang++','-std=c++17','-Wall','-Wextra','-Werror','-Wno-sign-compare','-DGRID_HOST_TEST',
            '-Icontroller/tests/mocks','-include','controller/tests/mocks/hardware.h',
            'controller/opengrid/src/main.cpp','controller/tests/test_feedback.cpp','-o',str(output)],cwd=ROOT,check=True)
        result=subprocess.run([str(output)],cwd=ROOT,text=True,capture_output=True,check=True)
        self.assertEqual(result.stdout.count('PASS '),4,result.stdout)
        print(result.stdout,end='')
if __name__=='__main__':unittest.main()
