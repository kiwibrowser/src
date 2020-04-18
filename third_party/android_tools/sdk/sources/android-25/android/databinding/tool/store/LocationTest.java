/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.databinding.tool.store;


import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;


public class LocationTest {
    @Test
    public void testInvalid() {
        assertFalse(new Location().isValid());
    }

    @Test
    public void testValid() {
        Location location = new Location(0, 0, 1, 1);
        assertTrue(location.isValid());
    }

    @Test
    public void testContains() {
        Location location1 = new Location(0, 0, 10, 1);
        Location location2 = new Location(0, 0, 9, 1);
        assertTrue(location1.contains(location2));
        location2.endLine = 10;
        assertTrue(location1.contains(location2));
        location2.endOffset = 2;
        assertFalse(location1.contains(location2));
    }

    @Test
    public void testAbsolute() {
        Location loc = new Location(1, 2, 3, 4);
        assertEquals(loc, loc.toAbsoluteLocation());
    }

    @Test
    public void testAbsoluteWithInvalidParent() {
        Location loc = new Location(1, 2, 3, 4);
        loc.setParentLocation(new Location());
        assertEquals(loc, loc.toAbsoluteLocation());
    }

    @Test
    public void testAbsoluteWithParent() {
        Location loc = new Location(1, 2, 3, 4);
        loc.setParentLocation(new Location(10, 0, 20, 0));
        assertEquals(new Location(11, 2, 13, 4), loc.toAbsoluteLocation());
    }

    @Test
    public void testAbsoluteWith2Parents() {
        Location loc = new Location(1, 2, 3, 4);
        Location parent1 = new Location(5, 6, 10, 11);
        parent1.setParentLocation(new Location(5, 6, 17, 8));
        loc.setParentLocation(parent1);
        assertEquals(new Location(10, 6, 15, 11), parent1.toAbsoluteLocation());
        assertEquals(new Location(11, 2, 13, 4), loc.toAbsoluteLocation());
    }

    @Test
    public void testAbsoluteWithSameLine() {
        Location loc = new Location(0, 2, 0, 4);
        loc.setParentLocation(new Location(7, 2, 12, 46));
        assertEquals(new Location(7, 4, 7, 6), loc.toAbsoluteLocation());
    }
}
