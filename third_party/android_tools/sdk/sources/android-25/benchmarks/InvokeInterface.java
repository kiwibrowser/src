/*
 * Copyright 2016 The Android Open Source Project
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

package benchmarks;

public class InvokeInterface {
  static MultiClass multi;

  static {
    multi = new MultiClass();
  }

  public void timeCall0Concrete(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall0Concrete(m);
    }
  }

  public void timeCall0Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall0Interface(m);
    }
  }

  public void timeCall1Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall1Interface(m);
    }
  }

  public void timeCall2Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall2Interface(m);
    }
  }

  public void timeCall3Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall3Interface(m);
    }
  }

  public void timeCall4Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall4Interface(m);
    }
  }

  public void timeCall5Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall5Interface(m);
    }
  }

  public void timeCall6Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall6Interface(m);
    }
  }

  public void timeCall7Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall7Interface(m);
    }
  }

  public void timeCall8Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall8Interface(m);
    }
  }

  public void timeCall9Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall9Interface(m);
    }
  }

  public void timeCall10Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall10Interface(m);
    }
  }

  public void timeCall11Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall11Interface(m);
    }
  }

  public void timeCall12Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall12Interface(m);
    }
  }

  public void timeCall13Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall13Interface(m);
    }
  }

  public void timeCall14Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall14Interface(m);
    }
  }

  public void timeCall15Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall15Interface(m);
    }
  }

  public void timeCall16Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall16Interface(m);
    }
  }

  public void timeCall17Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall17Interface(m);
    }
  }

  public void timeCall18Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall18Interface(m);
    }
  }

  public void timeCall19Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall19Interface(m);
    }
  }

  public void timeCall20Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall20Interface(m);
    }
  }

  public void timeCall21Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall21Interface(m);
    }
  }

  public void timeCall22Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall22Interface(m);
    }
  }

  public void timeCall23Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall23Interface(m);
    }
  }

  public void timeCall24Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall24Interface(m);
    }
  }

  public void timeCall25Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall25Interface(m);
    }
  }

  public void timeCall26Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall26Interface(m);
    }
  }

  public void timeCall27Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall27Interface(m);
    }
  }

  public void timeCall28Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall28Interface(m);
    }
  }

  public void timeCall29Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall29Interface(m);
    }
  }

  public void timeCall30Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall30Interface(m);
    }
  }

  public void timeCall31Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall31Interface(m);
    }
  }

  public void timeCall32Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall32Interface(m);
    }
  }

  public void timeCall33Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall33Interface(m);
    }
  }

  public void timeCall34Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall34Interface(m);
    }
  }

  public void timeCall35Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall35Interface(m);
    }
  }

  public void timeCall36Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall36Interface(m);
    }
  }

  public void timeCall37Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall37Interface(m);
    }
  }

  public void timeCall38Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall38Interface(m);
    }
  }

  public void timeCall39Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall39Interface(m);
    }
  }

  public void timeCall40Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall40Interface(m);
    }
  }

  public void timeCall41Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall41Interface(m);
    }
  }

  public void timeCall42Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall42Interface(m);
    }
  }

  public void timeCall43Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall43Interface(m);
    }
  }

  public void timeCall44Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall44Interface(m);
    }
  }

  public void timeCall45Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall45Interface(m);
    }
  }

  public void timeCall46Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall46Interface(m);
    }
  }

  public void timeCall47Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall47Interface(m);
    }
  }

  public void timeCall48Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall48Interface(m);
    }
  }

  public void timeCall49Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall49Interface(m);
    }
  }

  public void timeCall50Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall50Interface(m);
    }
  }

  public void timeCall51Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall51Interface(m);
    }
  }

  public void timeCall52Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall52Interface(m);
    }
  }

  public void timeCall53Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall53Interface(m);
    }
  }

  public void timeCall54Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall54Interface(m);
    }
  }

  public void timeCall55Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall55Interface(m);
    }
  }

  public void timeCall56Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall56Interface(m);
    }
  }

  public void timeCall57Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall57Interface(m);
    }
  }

  public void timeCall58Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall58Interface(m);
    }
  }

  public void timeCall59Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall59Interface(m);
    }
  }

  public void timeCall60Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall60Interface(m);
    }
  }

  public void timeCall61Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall61Interface(m);
    }
  }

  public void timeCall62Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall62Interface(m);
    }
  }

  public void timeCall63Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall63Interface(m);
    }
  }

  public void timeCall64Interface(int nreps) {
    MultiClass m = multi;
    for (int i = 0; i < nreps; i++) {
      doCall64Interface(m);
    }
  }

  // Try calling these through the IMT/IfTable
  public void doCall0Interface(Iface0 i) { i.callIface0(); }
  public void doCall1Interface(Iface1 i) { i.callIface1(); }
  public void doCall2Interface(Iface2 i) { i.callIface2(); }
  public void doCall3Interface(Iface3 i) { i.callIface3(); }
  public void doCall4Interface(Iface4 i) { i.callIface4(); }
  public void doCall5Interface(Iface5 i) { i.callIface5(); }
  public void doCall6Interface(Iface6 i) { i.callIface6(); }
  public void doCall7Interface(Iface7 i) { i.callIface7(); }
  public void doCall8Interface(Iface8 i) { i.callIface8(); }
  public void doCall9Interface(Iface9 i) { i.callIface9(); }
  public void doCall10Interface(Iface10 i) { i.callIface10(); }
  public void doCall11Interface(Iface11 i) { i.callIface11(); }
  public void doCall12Interface(Iface12 i) { i.callIface12(); }
  public void doCall13Interface(Iface13 i) { i.callIface13(); }
  public void doCall14Interface(Iface14 i) { i.callIface14(); }
  public void doCall15Interface(Iface15 i) { i.callIface15(); }
  public void doCall16Interface(Iface16 i) { i.callIface16(); }
  public void doCall17Interface(Iface17 i) { i.callIface17(); }
  public void doCall18Interface(Iface18 i) { i.callIface18(); }
  public void doCall19Interface(Iface19 i) { i.callIface19(); }
  public void doCall20Interface(Iface20 i) { i.callIface20(); }
  public void doCall21Interface(Iface21 i) { i.callIface21(); }
  public void doCall22Interface(Iface22 i) { i.callIface22(); }
  public void doCall23Interface(Iface23 i) { i.callIface23(); }
  public void doCall24Interface(Iface24 i) { i.callIface24(); }
  public void doCall25Interface(Iface25 i) { i.callIface25(); }
  public void doCall26Interface(Iface26 i) { i.callIface26(); }
  public void doCall27Interface(Iface27 i) { i.callIface27(); }
  public void doCall28Interface(Iface28 i) { i.callIface28(); }
  public void doCall29Interface(Iface29 i) { i.callIface29(); }
  public void doCall30Interface(Iface30 i) { i.callIface30(); }
  public void doCall31Interface(Iface31 i) { i.callIface31(); }
  public void doCall32Interface(Iface32 i) { i.callIface32(); }
  public void doCall33Interface(Iface33 i) { i.callIface33(); }
  public void doCall34Interface(Iface34 i) { i.callIface34(); }
  public void doCall35Interface(Iface35 i) { i.callIface35(); }
  public void doCall36Interface(Iface36 i) { i.callIface36(); }
  public void doCall37Interface(Iface37 i) { i.callIface37(); }
  public void doCall38Interface(Iface38 i) { i.callIface38(); }
  public void doCall39Interface(Iface39 i) { i.callIface39(); }
  public void doCall40Interface(Iface40 i) { i.callIface40(); }
  public void doCall41Interface(Iface41 i) { i.callIface41(); }
  public void doCall42Interface(Iface42 i) { i.callIface42(); }
  public void doCall43Interface(Iface43 i) { i.callIface43(); }
  public void doCall44Interface(Iface44 i) { i.callIface44(); }
  public void doCall45Interface(Iface45 i) { i.callIface45(); }
  public void doCall46Interface(Iface46 i) { i.callIface46(); }
  public void doCall47Interface(Iface47 i) { i.callIface47(); }
  public void doCall48Interface(Iface48 i) { i.callIface48(); }
  public void doCall49Interface(Iface49 i) { i.callIface49(); }
  public void doCall50Interface(Iface50 i) { i.callIface50(); }
  public void doCall51Interface(Iface51 i) { i.callIface51(); }
  public void doCall52Interface(Iface52 i) { i.callIface52(); }
  public void doCall53Interface(Iface53 i) { i.callIface53(); }
  public void doCall54Interface(Iface54 i) { i.callIface54(); }
  public void doCall55Interface(Iface55 i) { i.callIface55(); }
  public void doCall56Interface(Iface56 i) { i.callIface56(); }
  public void doCall57Interface(Iface57 i) { i.callIface57(); }
  public void doCall58Interface(Iface58 i) { i.callIface58(); }
  public void doCall59Interface(Iface59 i) { i.callIface59(); }
  public void doCall60Interface(Iface60 i) { i.callIface60(); }
  public void doCall61Interface(Iface61 i) { i.callIface61(); }
  public void doCall62Interface(Iface62 i) { i.callIface62(); }
  public void doCall63Interface(Iface63 i) { i.callIface63(); }
  public void doCall64Interface(Iface64 i) { i.callIface64(); }

  // Try calling this through the vtable for comparison.
  public void doCall0Concrete(MultiClass m) { m.callIface0(); }

  // IMTs are 64 entries in length. By making this 65 interfaces we guarantee that we will have a
  // collision.
  static class MultiClass implements Iface0, Iface1, Iface2, Iface3, Iface4,
                                     Iface5, Iface6, Iface7, Iface8, Iface9,
                                     Iface10, Iface11, Iface12, Iface13, Iface14,
                                     Iface15, Iface16, Iface17, Iface18, Iface19,
                                     Iface20, Iface21, Iface22, Iface23, Iface24,
                                     Iface25, Iface26, Iface27, Iface28, Iface29,
                                     Iface30, Iface31, Iface32, Iface33, Iface34,
                                     Iface35, Iface36, Iface37, Iface38, Iface39,
                                     Iface40, Iface41, Iface42, Iface43, Iface44,
                                     Iface45, Iface46, Iface47, Iface48, Iface49,
                                     Iface50, Iface51, Iface52, Iface53, Iface54,
                                     Iface55, Iface56, Iface57, Iface58, Iface59,
                                     Iface60, Iface61, Iface62, Iface63, Iface64 { }

  // The declaration of the 64 interfaces. We give them all default methods to avoid having to
  // repeat ourselves.
  static interface Iface0 { default void callIface0() {} }
  static interface Iface1 { default void callIface1() {} }
  static interface Iface2 { default void callIface2() {} }
  static interface Iface3 { default void callIface3() {} }
  static interface Iface4 { default void callIface4() {} }
  static interface Iface5 { default void callIface5() {} }
  static interface Iface6 { default void callIface6() {} }
  static interface Iface7 { default void callIface7() {} }
  static interface Iface8 { default void callIface8() {} }
  static interface Iface9 { default void callIface9() {} }
  static interface Iface10 { default void callIface10() {} }
  static interface Iface11 { default void callIface11() {} }
  static interface Iface12 { default void callIface12() {} }
  static interface Iface13 { default void callIface13() {} }
  static interface Iface14 { default void callIface14() {} }
  static interface Iface15 { default void callIface15() {} }
  static interface Iface16 { default void callIface16() {} }
  static interface Iface17 { default void callIface17() {} }
  static interface Iface18 { default void callIface18() {} }
  static interface Iface19 { default void callIface19() {} }
  static interface Iface20 { default void callIface20() {} }
  static interface Iface21 { default void callIface21() {} }
  static interface Iface22 { default void callIface22() {} }
  static interface Iface23 { default void callIface23() {} }
  static interface Iface24 { default void callIface24() {} }
  static interface Iface25 { default void callIface25() {} }
  static interface Iface26 { default void callIface26() {} }
  static interface Iface27 { default void callIface27() {} }
  static interface Iface28 { default void callIface28() {} }
  static interface Iface29 { default void callIface29() {} }
  static interface Iface30 { default void callIface30() {} }
  static interface Iface31 { default void callIface31() {} }
  static interface Iface32 { default void callIface32() {} }
  static interface Iface33 { default void callIface33() {} }
  static interface Iface34 { default void callIface34() {} }
  static interface Iface35 { default void callIface35() {} }
  static interface Iface36 { default void callIface36() {} }
  static interface Iface37 { default void callIface37() {} }
  static interface Iface38 { default void callIface38() {} }
  static interface Iface39 { default void callIface39() {} }
  static interface Iface40 { default void callIface40() {} }
  static interface Iface41 { default void callIface41() {} }
  static interface Iface42 { default void callIface42() {} }
  static interface Iface43 { default void callIface43() {} }
  static interface Iface44 { default void callIface44() {} }
  static interface Iface45 { default void callIface45() {} }
  static interface Iface46 { default void callIface46() {} }
  static interface Iface47 { default void callIface47() {} }
  static interface Iface48 { default void callIface48() {} }
  static interface Iface49 { default void callIface49() {} }
  static interface Iface50 { default void callIface50() {} }
  static interface Iface51 { default void callIface51() {} }
  static interface Iface52 { default void callIface52() {} }
  static interface Iface53 { default void callIface53() {} }
  static interface Iface54 { default void callIface54() {} }
  static interface Iface55 { default void callIface55() {} }
  static interface Iface56 { default void callIface56() {} }
  static interface Iface57 { default void callIface57() {} }
  static interface Iface58 { default void callIface58() {} }
  static interface Iface59 { default void callIface59() {} }
  static interface Iface60 { default void callIface60() {} }
  static interface Iface61 { default void callIface61() {} }
  static interface Iface62 { default void callIface62() {} }
  static interface Iface63 { default void callIface63() {} }
  static interface Iface64 { default void callIface64() {} }
}
