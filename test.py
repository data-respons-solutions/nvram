#!/usr/bin/python3

import unittest
import tempfile
import os
import subprocess
from subprocess import CalledProcessError

def nvram(env, arglist):
    args = ['./nvram']
    args.extend(arglist)
    r = subprocess.run(args, capture_output=True, text=True, env=env, check=True)
    return r.stdout

def nvram_set(env, key, val):
    nvram(env, ['set', key, val])

def nvram_get(env, key):
    return nvram(env, ['get', key]).rstrip()

def nvram_list(env):
    stdout = nvram(env, ['list'])
    return dict(pair.split("=") for pair in stdout.split())

class test_user_base(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.dir = self.tmpdir.name
        self.env = {
                'NVRAM_SYSTEM_A': f'{self.dir}/system_a',
                'NVRAM_SYSTEM_B': f'{self.dir}/system_b',
                'NVRAM_USER_A': f'{self.dir}/user_a',
                'NVRAM_USER_B': f'{self.dir}/user_b',
            }
    
    def tearDown(self):
        self.assertFalse(os.path.isfile(self.env['NVRAM_SYSTEM_A']))
        self.assertFalse(os.path.isfile(self.env['NVRAM_SYSTEM_B']))
        self.tmpdir.cleanup()

class test_user_set_get(test_user_base):
    def test_set_get(self):
        nvram_set(self.env, 'var1', 'val1')
        var1 = nvram_get(self.env, 'var1')
        self.assertEqual(var1, 'val1')
        
    def test_set_get_multiple(self):
        attributes = {}
        for i in range(10):
            key = f'key{i}'
            val = f'val{i}'
            attributes[key] = val
            nvram_set(self.env, key, val)
        
        for key, value in attributes.items():
            read = nvram_get(self.env, key)
            self.assertEqual(read, value)
            
    def test_get_empty(self):
        with self.assertRaises(CalledProcessError):
            nvram_get(self.env, 'key1')
        
class test_user_list(test_user_base):
    def test_list(self):
        attributes = {}
        for i in range(10):
            key = f'key{i}'
            val = f'val{i}'
            attributes[key] = val
            nvram_set(self.env, key, val)
        d = nvram_list(self.env)
        self.assertEqual(d, attributes)
        
    def test_empty(self):
        d = nvram_list(self.env)
        self.assertEqual(0, len(d))
        
if __name__ == '__main__':
    unittest.main()