This directory contains generic tests for fragmentation.

Spec: https://www.w3.org/TR/2016/CR-css-break-3-20160114/

Actual layout tests here will typically use multicol, paged overflow or printing to test some
fragmentation behavior, but they should not test anything pertaining to a specific fragmentation
type. Such specific tests should go in printing/ , fast/pagination/ or fast/multicol/ .
