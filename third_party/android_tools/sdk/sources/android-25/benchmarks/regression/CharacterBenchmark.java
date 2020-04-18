/*
 * Copyright (C) 2009 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package benchmarks.regression;

import com.google.caliper.BeforeExperiment;
import com.google.caliper.Param;

/**
 * Tests various Character methods, intended for testing multiple
 * implementations against each other.
 */
public class CharacterBenchmark {

    @Param private CharacterSet characterSet;

    @Param private Overload overload;

    private char[] chars;

    @BeforeExperiment
    protected void setUp() throws Exception {
        this.chars = characterSet.chars;
    }

    public enum Overload { CHAR, INT }

    public double nanosToUnits(double nanos) {
        return nanos / 65536;
    }

    public enum CharacterSet {
        ASCII(128),
        UNICODE(65536);
        final char[] chars;
        CharacterSet(int size) {
            this.chars = new char[65536];
            for (int i = 0; i < 65536; ++i) {
                chars[i] = (char) (i % size);
            }
        }
    }

    // A fake benchmark to give us a baseline.
    public boolean timeIsSpace(int reps) {
        boolean dummy = false;
        if (overload == Overload.CHAR) {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    dummy ^= ((char) ch == ' ');
                }
            }
        } else {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    dummy ^= (ch == ' ');
                }
            }
        }
        return dummy;
    }

    public void timeDigit(int reps) {
        if (overload == Overload.CHAR) {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.digit(chars[ch], 10);
                }
            }
        } else {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.digit((int) chars[ch], 10);
                }
            }
        }
    }

    public void timeGetNumericValue(int reps) {
        if (overload == Overload.CHAR) {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.getNumericValue(chars[ch]);
                }
            }
        } else {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.getNumericValue((int) chars[ch]);
                }
            }
        }
    }

    public void timeIsDigit(int reps) {
        if (overload == Overload.CHAR) {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isDigit(chars[ch]);
                }
            }
        } else {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isDigit((int) chars[ch]);
                }
            }
        }
    }

    public void timeIsIdentifierIgnorable(int reps) {
        if (overload == Overload.CHAR) {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isIdentifierIgnorable(chars[ch]);
                }
            }
        } else {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isIdentifierIgnorable((int) chars[ch]);
                }
            }
        }
    }

    public void timeIsJavaIdentifierPart(int reps) {
        if (overload == Overload.CHAR) {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isJavaIdentifierPart(chars[ch]);
                }
            }
        } else {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isJavaIdentifierPart((int) chars[ch]);
                }
            }
        }
    }

    public void timeIsJavaIdentifierStart(int reps) {
        if (overload == Overload.CHAR) {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isJavaIdentifierStart(chars[ch]);
                }
            }
        } else {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isJavaIdentifierStart((int) chars[ch]);
                }
            }
        }
    }

    public void timeIsLetter(int reps) {
        if (overload == Overload.CHAR) {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isLetter(chars[ch]);
                }
            }
        } else {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isLetter((int) chars[ch]);
                }
            }
        }
    }

    public void timeIsLetterOrDigit(int reps) {
        if (overload == Overload.CHAR) {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isLetterOrDigit(chars[ch]);
                }
            }
        } else {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isLetterOrDigit((int) chars[ch]);
                }
            }
        }
    }

    public void timeIsLowerCase(int reps) {
        if (overload == Overload.CHAR) {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isLowerCase(chars[ch]);
                }
            }
        } else {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isLowerCase((int) chars[ch]);
                }
            }
        }
    }

    public void timeIsSpaceChar(int reps) {
        if (overload == Overload.CHAR) {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isSpaceChar(chars[ch]);
                }
            }
        } else {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isSpaceChar((int) chars[ch]);
                }
            }
        }
    }

    public void timeIsUpperCase(int reps) {
        if (overload == Overload.CHAR) {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isUpperCase(chars[ch]);
                }
            }
        } else {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isUpperCase((int) chars[ch]);
                }
            }
        }
    }

    public void timeIsWhitespace(int reps) {
        if (overload == Overload.CHAR) {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isWhitespace(chars[ch]);
                }
            }
        } else {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.isWhitespace((int) chars[ch]);
                }
            }
        }
    }

    public void timeToLowerCase(int reps) {
        if (overload == Overload.CHAR) {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.toLowerCase(chars[ch]);
                }
            }
        } else {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.toLowerCase((int) chars[ch]);
                }
            }
        }
    }

    public void timeToUpperCase(int reps) {
        if (overload == Overload.CHAR) {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.toUpperCase(chars[ch]);
                }
            }
        } else {
            for (int i = 0; i < reps; ++i) {
                for (int ch = 0; ch < 65536; ++ch) {
                    Character.toUpperCase((int) chars[ch]);
                }
            }
        }
    }
}
