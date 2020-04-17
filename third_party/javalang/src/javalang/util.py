

class LookAheadIterator(object):
    def __init__(self, iterable):
        self.iterable = iter(iterable)
        self.look_ahead = list()
        self.markers = list()
        self.default = None
        self.value = None

    def __iter__(self):
        return self

    def set_default(self, value):
        self.default = value

    def next(self):
        return self.__next__()

    def __next__(self):
        if self.look_ahead:
            self.value = self.look_ahead.pop(0)
        else:
            self.value = next(self.iterable)

        if self.markers:
            self.markers[-1].append(self.value)

        return self.value

    def look(self, i=0):
        """ Look ahead of the iterable by some number of values with advancing
        past them.

        If the requested look ahead is past the end of the iterable then None is
        returned.

        """

        length = len(self.look_ahead)

        if length <= i:
            try:
                self.look_ahead.extend([next(self.iterable)
                    for _ in range(length, i + 1)])
            except StopIteration:
                return self.default

        self.value = self.look_ahead[i]
        return self.value

    def last(self):
        return self.value

    def __enter__(self):
        self.push_marker()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        # Reset the iterator if there was an error
        if exc_type or exc_val or exc_tb:
            self.pop_marker(True)
        else:
            self.pop_marker(False)

    def push_marker(self):
        """ Push a marker on to the marker stack """
        self.markers.append(list())

    def pop_marker(self, reset):
        """ Pop a marker off of the marker stack. If reset is True then the
        iterator will be returned to the state it was in before the
        corresponding call to push_marker().

        """

        marker = self.markers.pop()

        if reset:
            # Make the values available to be read again
            marker.extend(self.look_ahead)
            self.look_ahead = marker
        elif self.markers:
            # Otherwise, reassign the values to the top marker
            self.markers[-1].extend(marker)
        else:
            # If there are not more markers in the stack then discard the values
            pass

class LookAheadListIterator(object):
    def __init__(self, iterable):
        self.list = list(iterable)

        self.marker = 0
        self.saved_markers = []

        self.default = None
        self.value = None

    def __iter__(self):
        return self

    def set_default(self, value):
        self.default = value

    def next(self):
        return self.__next__()

    def __next__(self):
        try:
            self.value = self.list[self.marker]
            self.marker += 1
        except IndexError:
            raise StopIteration()

        return self.value

    def look(self, i=0):
        """ Look ahead of the iterable by some number of values with advancing
        past them.

        If the requested look ahead is past the end of the iterable then None is
        returned.

        """

        try:
            self.value = self.list[self.marker + i]
        except IndexError:
            return self.default

        return self.value

    def last(self):
        return self.value

    def __enter__(self):
        self.push_marker()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        # Reset the iterator if there was an error
        if exc_type or exc_val or exc_tb:
            self.pop_marker(True)
        else:
            self.pop_marker(False)

    def push_marker(self):
        """ Push a marker on to the marker stack """
        self.saved_markers.append(self.marker)

    def pop_marker(self, reset):
        """ Pop a marker off of the marker stack. If reset is True then the
        iterator will be returned to the state it was in before the
        corresponding call to push_marker().

        """

        saved = self.saved_markers.pop()

        if reset:
            self.marker = saved
        elif self.saved_markers:
            self.saved_markers[-1] = saved

