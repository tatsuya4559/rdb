import subprocess
from unittest import TestCase


class DBTest(TestCase):
    def run_script(self, commands: list[str]) -> list[str]:
        input_data = "\n".join(commands) + "\n"

        p = subprocess.Popen(
            "./db",
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        stdout, stderr = p.communicate(input=input_data)

        if p.returncode != 0:
            print(stderr)
            self.fail()

        return stdout.split("\n")

    def test_normal_insert_and_select(self):
        commands = [
            "insert 1 user1 person1@example.com",
            "select",
            ".exit",
        ]
        want = [
            "db> Executed.",
            "db> (1, user1, person1@example.com)",
            "Executed.",
            "db> ",
        ]
        got = self.run_script(commands)
        self.assertEqual(got, want)
