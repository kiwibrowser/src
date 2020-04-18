/*
 * Written by Doug Lea with assistance from members of JCP JSR-166
 * Expert Group and released to the public domain, as explained at
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

package jsr166;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingDeque;

public class LinkedBlockingDequeUnboundedTest extends BlockingQueueTest {

    protected BlockingQueue emptyCollection() {
        return new LinkedBlockingDeque();
    }

}
