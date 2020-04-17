import unittest

from ..util import LookAheadIterator


class TestLookAheadIterator(unittest.TestCase):
    def test_usage(self):
        i = LookAheadIterator(list(range(0, 10000)))

        self.assertEqual(next(i), 0)
        self.assertEqual(next(i), 1)
        self.assertEqual(next(i), 2)

        self.assertEqual(i.last(), 2)

        self.assertEqual(i.look(), 3)
        self.assertEqual(i.last(), 3)

        self.assertEqual(i.look(1), 4)
        self.assertEqual(i.look(2), 5)
        self.assertEqual(i.look(3), 6)
        self.assertEqual(i.look(4), 7)

        self.assertEqual(i.last(), 7)

        i.push_marker()
        self.assertEqual(next(i), 3)
        self.assertEqual(next(i), 4)
        self.assertEqual(next(i), 5)
        i.pop_marker(True) # reset

        self.assertEqual(i.look(), 3)
        self.assertEqual(next(i), 3)

        i.push_marker() #1
        self.assertEqual(next(i), 4)
        self.assertEqual(next(i), 5)
        i.push_marker() #2
        self.assertEqual(next(i), 6)
        self.assertEqual(next(i), 7)
        i.push_marker() #3
        self.assertEqual(next(i), 8)
        self.assertEqual(next(i), 9)
        i.pop_marker(False) #3
        self.assertEqual(next(i), 10)
        i.pop_marker(True) #2
        self.assertEqual(next(i), 6)
        self.assertEqual(next(i), 7)
        self.assertEqual(next(i), 8)
        i.pop_marker(False) #1
        self.assertEqual(next(i), 9)

        try:
            with i:
                self.assertEqual(next(i), 10)
                self.assertEqual(next(i), 11)
                raise Exception()
        except:
            self.assertEqual(next(i), 10)
            self.assertEqual(next(i), 11)

        with i:
            self.assertEqual(next(i), 12)
            self.assertEqual(next(i), 13)
        self.assertEqual(next(i), 14)


if __name__=="__main__":
    unittest.main()
