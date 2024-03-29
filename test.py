import os
import subprocess
from unittest import TestCase


class DBTest(TestCase):
    TEST_DB = "test.db"

    def tearDown(self):
        os.remove(self.TEST_DB)

    def run_commands(self, commands: list[str]) -> list[str]:
        input_data = "\n".join(commands) + "\n"

        p = subprocess.Popen(
            ["./db", self.TEST_DB],
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
        got = self.run_commands(commands)
        self.assertEqual(got, want)

    def test_error_when_table_is_full(self):
        commands = [
            f"insert {i} user{i} person{i}@example.com"
            for i in range(6002)
        ]
        commands.append(".exit")
        want = "db> Error: Table full."

        got = self.run_commands(commands)
        self.assertEqual(got[-2], want)

    def test_insert_max_len_string(self):
        long_username = "u" * 32
        long_email = "e" * 255
        commands = [
            f"insert 1 {long_username} {long_email}",
            "select",
            ".exit",
        ]
        want = [
            "db> Executed.",
            f"db> (1, {long_username}, {long_email})",
            "Executed.",
            "db> ",
        ]
        got = self.run_commands(commands)
        self.assertEqual(got, want)

    def test_error_when_strings_too_long(self):
        long_username = "u" * 33
        long_email = "e" * 256
        commands = [
            f"insert 1 {long_username} {long_email}",
            "select",
            ".exit",
        ]
        want = [
            "db> String is too long.",
            "db> Executed.",
            "db> ",
        ]
        got = self.run_commands(commands)
        self.assertEqual(got, want)

    def test_error_when_id_is_negative(self):
        commands = [
            "insert -1 foo foo@example.com",
            "select",
            ".exit",
        ]
        want = [
            "db> ID must be positive.",
            "db> Executed.",
            "db> ",
        ]
        got = self.run_commands(commands)
        self.assertEqual(got, want)

    def test_keep_data_after_closing_connection(self):
        self.run_commands([
            "insert 1 user1 person1@example.com",
            ".exit",
        ])

        want = [
            "db> (1, user1, person1@example.com)",
            "Executed.",
            "db> ",
        ]
        got = self.run_commands([
            "select",
            ".exit",
        ])
        self.assertEqual(got, want)

    def test_btree_visalization(self):
        want = [
            "db> Executed.",
            "db> Executed.",
            "db> Tree:",
            "- leaf (size 2)",
            "  - 1",
            "  - 3",
            "db> ",
        ]
        got = self.run_commands([
            "insert 3 user1 person1@example.com",
            "insert 1 user2 person2@example.com",
            ".btree",
            ".exit",
        ])
        self.assertEqual(got, want)
