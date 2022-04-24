#include "src/builtins/builtins-string-gen.h"
#include "src/builtins/builtins-regexp-gen.h"
#include "src/builtins/builtins-utils-gen.h"
#include "src/builtins/builtins.h"
#include "src/code-stub-assembler.h"
#include "src/code-factory.h"
#include "src/heap/factory-inl.h"
#include "src/objects.h"


#include <type_traits>

template <class T>
struct Identity {
  using type = T;
};

template <class T>
struct UnderlyingTypeHelper : Identity<typename std::underlying_type<T>::type> {
};

template <class T>
using UnderlyingTypeIfEnum =
    typename std::conditional_t<std::is_enum<T>::value, UnderlyingTypeHelper<T>,
                                Identity<T>>::type;

// Utility for extracting the underlying type of an enum, returns the type
// itself if not an enum.
template <class T>
UnderlyingTypeIfEnum<T> CastToUnderlyingTypeIfEnum(T x) {
  return static_cast<UnderlyingTypeIfEnum<T>>(x);
}

namespace v8 {
namespace internal {
	using compiler::Node;
	typedef compiler::Node Node;
	template <class T>
		using TNode = compiler::TNode<T>;
TNode<BoolT> FromConstexpr_bool_constexpr_bool_0(compiler::CodeAssemblerState* state_, bool p_b);
TNode<Number> Convert_Number_Smi_0(compiler::CodeAssemblerState* state_, TNode<Smi> p_i);
TNode<Oddball> True_0(compiler::CodeAssemblerState* state_) {
  compiler::CodeAssembler ca_(state_);
  compiler::CodeAssemblerParameterizedLabel<> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0);

    ca_.Bind(&block0);
//    ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 544);
  TNode<Oddball> tmp0;
    tmp0 = CodeStubAssembler(state_).TrueConstant();
  return TNode<Oddball>{tmp0};}

TNode<Oddball> Null_0(compiler::CodeAssemblerState* state_) {
  compiler::CodeAssembler ca_(state_);
  compiler::CodeAssemblerParameterizedLabel<> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0);

    ca_.Bind(&block0);
//    ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 542);
  TNode<Oddball> tmp0;
    tmp0 = CodeStubAssembler(state_).NullConstant();
  return TNode<Oddball>{tmp0};}

TNode<Oddball> Undefined_0(compiler::CodeAssemblerState* state_) {
  compiler::CodeAssembler ca_(state_);
  compiler::CodeAssemblerParameterizedLabel<> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0);

    ca_.Bind(&block0);
   // ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 543);
  TNode<Oddball> tmp0;
    tmp0 = CodeStubAssembler(state_).UndefinedConstant();
  return TNode<Oddball>{tmp0};}
/*
TNode<String> Cast_String_0(compiler::CodeAssemblerState* state_, TNode<HeapObject> p_obj, compiler::CodeAssemblerLabel* label_CastError) {
  compiler::CodeAssembler ca_(state_);
//  compiler::CodeAssembler::SourcePositionScope pos_scope(&ca_);
  compiler::CodeAssemblerParameterizedLabel<> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block4(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block3(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block5(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0);

  TNode<String> tmp0;
  if (block0.is_used()) {
    ca_.Bind(&block0);
  //  ca_.SetSourcePosition("../../v8/src/objects/string.tq", 7);
    compiler::CodeAssemblerLabel label1(&ca_);
    tmp0 = DownCastForTorqueClass_String_0(state_, TNode<HeapObject>{p_obj}, &label1);
    ca_.Goto(&block3);
    if (label1.is_used()) {
      ca_.Bind(&label1);
      ca_.Goto(&block4);
    }
  }

  if (block4.is_used()) {
    ca_.Bind(&block4);
    ca_.Goto(label_CastError);
  }

  if (block3.is_used()) {
    ca_.Bind(&block3);
    ca_.Goto(&block5);
  }

    ca_.Bind(&block5);
  return TNode<String>{tmp0};
}
// https://source.chromium.org/chromium/chromium/src/+/main:v8/src/builtins/array-join.tq?l=247&c=7
TNode<String> Cast_String_1(compiler::CodeAssemblerState* state_, TNode<Context> p_context, TNode<Object> p_o, compiler::CodeAssemblerLabel* label_CastError) {
  compiler::CodeAssembler ca_(state_);
  //compiler::CodeAssembler::SourcePositionScope pos_scope(&ca_);
  compiler::CodeAssemblerParameterizedLabel<> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block4(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block3(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block6(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block5(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block1(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block7(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0);

  TNode<HeapObject> tmp0;
  if (block0.is_used()) {
    ca_.Bind(&block0);
  //  ca_.SetSourcePosition("../../v8/src/builtins/cast.tq", 162);
    compiler::CodeAssemblerLabel label1(&ca_);
    tmp0 = CodeStubAssembler(state_).TaggedToHeapObject(TNode<Object>{p_o}, &label1);
    ca_.Goto(&block3);
    if (label1.is_used()) {
      ca_.Bind(&label1);
      ca_.Goto(&block4);
    }
  }

  if (block4.is_used()) {
    ca_.Bind(&block4);
    ca_.Goto(&block1);
  }

  TNode<String> tmp2;
  if (block3.is_used()) {
    ca_.Bind(&block3);
    compiler::CodeAssemblerLabel label3(&ca_);
    tmp2 = Cast_String_0(state_, TNode<HeapObject>{tmp0}, &label3);
    ca_.Goto(&block5);
    if (label3.is_used()) {
      ca_.Bind(&label3);
      ca_.Goto(&block6);
    }
  }

  if (block6.is_used()) {
    ca_.Bind(&block6);
    ca_.Goto(&block1);
  }

  if (block5.is_used()) {
    ca_.Bind(&block5);
//    ca_.SetSourcePosition("../../v8/src/builtins/array-join.tq", 247);
    ca_.Goto(&block7);
  }

  if (block1.is_used()) {
    ca_.Bind(&block1);
    ca_.Goto(label_CastError);
  }

    ca_.Bind(&block7);
  return TNode<String>{tmp2};
}

TNode<BoolT> Is_String_Object_0(compiler::CodeAssemblerState* state_, TNode<Context> p_context, TNode<Object> p_o) {
  compiler::CodeAssembler ca_(state_);
  //compiler::CodeAssembler::SourcePositionScope pos_scope(&ca_);
  compiler::CodeAssemblerParameterizedLabel<> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block5(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block4(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<BoolT> block1(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block6(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0);

  TNode<String> tmp0;
  if (block0.is_used()) {
    ca_.Bind(&block0);
   // ca_.SetSourcePosition("../../v8/src/builtins/cast.tq", 793);
    compiler::CodeAssemblerLabel label1(&ca_);
    tmp0 = Cast_String_1(state_, TNode<Context>{p_context}, TNode<Object>{p_o}, &label1);
    ca_.Goto(&block4);
    if (label1.is_used()) {
      ca_.Bind(&label1);
      ca_.Goto(&block5);
    }
  }

  TNode<BoolT> tmp2;
  if (block5.is_used()) {
    ca_.Bind(&block5);
    tmp2 = FromConstexpr_bool_constexpr_bool_0(state_, false);
    ca_.Goto(&block1, tmp2);
  }

  TNode<BoolT> tmp3;
  if (block4.is_used()) {
    ca_.Bind(&block4);
  //  ca_.SetSourcePosition("../../v8/src/builtins/cast.tq", 794);
    tmp3 = FromConstexpr_bool_constexpr_bool_0(state_, true);
    ca_.Goto(&block1, tmp3);
  }

  TNode<BoolT> phi_bb1_2;
  if (block1.is_used()) {
    ca_.Bind(&block1, &phi_bb1_2);
//    ca_.SetSourcePosition("../../v8/src/builtins/cast.tq", 799);
    ca_.Goto(&block6);
  }

    ca_.Bind(&block6);
  return TNode<BoolT>{phi_bb1_2};
}

*/
TNode<String> UnsafeCast_String_0(compiler::CodeAssemblerState* state_, TNode<Context> p_context, TNode<Object> p_o) {
  compiler::CodeAssembler ca_(state_);
  compiler::CodeAssemblerParameterizedLabel<Context, Object> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object> block3(&ca_, compiler::CodeAssemblerLabel::kDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object> block2(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, String> block1(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, String> block4(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0, p_context, p_o);

  if (block0.is_used()) {
    TNode<Context> tmp0;
    TNode<Object> tmp1;
    ca_.Bind(&block0, &tmp0, &tmp1);
    //ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 3181);
    TNode<BoolT> tmp2;
    USE(tmp2);
    tmp2 = CodeStubAssembler(state_).IsString(CodeStubAssembler(state_).Cast(TNode<Object>{tmp1}));
    ca_.Branch(tmp2, &block2, &block3, tmp0, tmp1);
  }

  if (block3.is_used()) {
    TNode<Context> tmp3;
    TNode<Object> tmp4;
    ca_.Bind(&block3, &tmp3, &tmp4);
    CodeStubAssembler(state_).FailAssert("Torque assert 'Is<A>(o)' failed", "src/builtins/base.tq", 3181);
  }

  if (block2.is_used()) {
    TNode<Context> tmp5;
    TNode<Object> tmp6;
    ca_.Bind(&block2, &tmp5, &tmp6);
  //  ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 3182);
    TNode<String> tmp7;
    USE(tmp7);
    tmp7 = TORQUE_CAST(TNode<Object>{tmp6});
    ca_.Goto(&block1, tmp5, tmp6, tmp7);
  }

  if (block1.is_used()) {
    TNode<Context> tmp8;
    TNode<Object> tmp9;
    TNode<String> tmp10;
    ca_.Bind(&block1, &tmp8, &tmp9, &tmp10);
//    ca_.SetSourcePosition("../../v8/src/builtins/regexp-match-all.tq", 194);
    ca_.Goto(&block4, tmp8, tmp9, tmp10);
  }

    TNode<Context> tmp11;
    TNode<Object> tmp12;
    TNode<String> tmp13;
    ca_.Bind(&block4, &tmp11, &tmp12, &tmp13);
  return TNode<String>{tmp13};
}

TNode<JSReceiver> UnsafeCast_Callable_0(compiler::CodeAssemblerState* state_, TNode<Context> p_context, TNode<Object> p_o) {
  compiler::CodeAssembler ca_(state_);
  compiler::CodeAssemblerParameterizedLabel<Context, Object> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object> block3(&ca_, compiler::CodeAssemblerLabel::kDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object> block2(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, JSReceiver> block4(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0, p_context, p_o);

  if (block0.is_used()) {
    TNode<Context> tmp0;
    TNode<Object> tmp1;
    ca_.Bind(&block0, &tmp0, &tmp1);
 //   ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 859);
    TNode<BoolT> tmp2;
    USE(tmp2);
    CodeStubAssembler csa_(state_);
    tmp2 = csa_.IsCallable(csa_.Cast( TNode<Object>{tmp1}));
    ca_.Branch(tmp2, &block2, &block3, tmp0, tmp1);
  }

  if (block3.is_used()) {
    TNode<Context> tmp3;
    TNode<Object> tmp4;
    ca_.Bind(&block3, &tmp3, &tmp4);
    CodeStubAssembler(state_).FailAssert("Torque assert 'Is<A>(o)' failed", "src/builtins/base.tq", 859);
  }

  if (block2.is_used()) {
    TNode<Context> tmp5;
    TNode<Object> tmp6;
    ca_.Bind(&block2, &tmp5, &tmp6);
   // ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 860);
    TNode<JSReceiver> tmp7;
    USE(tmp7);
    tmp7 = TORQUE_CAST(TNode<Object>{tmp6});
   // ca_.SetSourcePosition("../../v8/src/builtins/array-from.tq", 111);
    ca_.Goto(&block4, tmp5, tmp6, tmp7);
  }

    TNode<Context> tmp8;
    TNode<Object> tmp9;
    TNode<JSReceiver> tmp10;
    ca_.Bind(&block4, &tmp8, &tmp9, &tmp10);
  return TNode<JSReceiver>{tmp10};
}

TNode<JSReceiver> Cast_Callable_0(compiler::CodeAssemblerState* state_, TNode<HeapObject> p_o, compiler::CodeAssemblerLabel* label_CastError) {
  compiler::CodeAssembler ca_(state_);
  //compiler::CodeAssembler::SourcePositionScope pos_scope(&ca_);
  compiler::CodeAssemblerParameterizedLabel<> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block4(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block3(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block5(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0);

  TNode<JSReceiver> tmp0;
  if (block0.is_used()) {
    ca_.Bind(&block0);
   // ca_.SetSourcePosition("../../v8/src/builtins/cast.tq", 403);
    compiler::CodeAssemblerLabel label1(&ca_);
    tmp0 = CodeStubAssembler(state_).HeapObjectToCallable(TNode<HeapObject>{p_o}, &label1);
    ca_.Goto(&block3);
    if (label1.is_used()) {
      ca_.Bind(&label1);
      ca_.Goto(&block4);
    }
  }

  if (block4.is_used()) {
    ca_.Bind(&block4);
  //  ca_.SetSourcePosition("../../v8/src/builtins/cast.tq", 401);
    ca_.Goto(label_CastError);
  }

  if (block3.is_used()) {
    ca_.Bind(&block3);
    ca_.Goto(&block5);
  }

    ca_.Bind(&block5);
  return TNode<JSReceiver>{tmp0};
}

// https://source.chromium.org/chromium/chromium/src/+/main:v8/src/builtins/array-every.tq?l=18&c=22
TNode<JSReceiver> Cast_Callable_1(compiler::CodeAssemblerState* state_, TNode<Context> p_context, TNode<Object> p_o, compiler::CodeAssemblerLabel* label_CastError) {
  compiler::CodeAssembler ca_(state_);
//  compiler::CodeAssembler::SourcePositionScope pos_scope(&ca_);
  compiler::CodeAssemblerParameterizedLabel<> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block4(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block3(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block6(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block5(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block1(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block7(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0);

  TNode<HeapObject> tmp0;
  if (block0.is_used()) {
    ca_.Bind(&block0);
  //  ca_.SetSourcePosition("../../v8/src/builtins/cast.tq", 162);
    compiler::CodeAssemblerLabel label1(&ca_);
    tmp0 = CodeStubAssembler(state_).TaggedToHeapObject(TNode<Object>{p_o}, &label1);
    ca_.Goto(&block3);
    if (label1.is_used()) {
      ca_.Bind(&label1);
      ca_.Goto(&block4);
    }
  }

  if (block4.is_used()) {
    ca_.Bind(&block4);
    ca_.Goto(&block1);
  }

  TNode<JSReceiver> tmp2;
  if (block3.is_used()) {
    ca_.Bind(&block3);
    compiler::CodeAssemblerLabel label3(&ca_);
    tmp2 = Cast_Callable_0(state_, TNode<HeapObject>{tmp0}, &label3);
    ca_.Goto(&block5);
    if (label3.is_used()) {
      ca_.Bind(&label3);
      ca_.Goto(&block6);
    }
  }

  if (block6.is_used()) {
    ca_.Bind(&block6);
    ca_.Goto(&block1);
  }

  if (block5.is_used()) {
    ca_.Bind(&block5);
//    ca_.SetSourcePosition("../../v8/src/builtins/array-every.tq", 18);
    ca_.Goto(&block7);
  }

  if (block1.is_used()) {
    ca_.Bind(&block1);
    ca_.Goto(label_CastError);
  }

    ca_.Bind(&block7);
  return TNode<JSReceiver>{tmp2};
}



TNode<JSReceiver> GetMethod_3(compiler::CodeAssemblerState* state_, TNode<Context> p_context, TNode<Object> p_o, TNode<Symbol> p_symbol, compiler::CodeAssemblerLabel* label_IfNullOrUndefined) {
  compiler::CodeAssembler ca_(state_);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Symbol> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Symbol, Object, BoolT> block5(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Symbol, Object, BoolT> block6(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Symbol, Object, BoolT, BoolT> block7(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Symbol, Object> block3(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Symbol, Object> block4(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Symbol, Object, Object, Context> block11(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Symbol, Object, Object, Context, JSReceiver> block10(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Symbol, JSReceiver> block12(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0, p_context, p_o, p_symbol);

  if (block0.is_used()) {
    TNode<Context> tmp0;
    TNode<Object> tmp1;
    TNode<Symbol> tmp2;
    ca_.Bind(&block0, &tmp0, &tmp1, &tmp2);
    //ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 1244);
    TNode<Object> tmp3;
    USE(tmp3);
    tmp3 = CodeStubAssembler(state_).GetProperty(TNode<Context>{tmp0}, TNode<Object>{tmp1}, TNode<Object>{tmp2});
    //ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 1245);
    TNode<Oddball> tmp4;
    USE(tmp4);
    tmp4 = Undefined_0(state_);
    TNode<BoolT> tmp5;
    USE(tmp5);
    tmp5 = CodeStubAssembler(state_).TaggedEqual(TNode<Object>{tmp3}, TNode<HeapObject>{tmp4});
    ca_.Branch(tmp5, &block5, &block6, tmp0, tmp1, tmp2, tmp3, tmp5);
  }

  if (block5.is_used()) {
    TNode<Context> tmp6;
    TNode<Object> tmp7;
    TNode<Symbol> tmp8;
    TNode<Object> tmp9;
    TNode<BoolT> tmp10;
    ca_.Bind(&block5, &tmp6, &tmp7, &tmp8, &tmp9, &tmp10);
    TNode<BoolT> tmp11;
    USE(tmp11);
    tmp11 = FromConstexpr_bool_constexpr_bool_0(state_, true);
    ca_.Goto(&block7, tmp6, tmp7, tmp8, tmp9, tmp10, tmp11);
  }

  if (block6.is_used()) {
    TNode<Context> tmp12;
    TNode<Object> tmp13;
    TNode<Symbol> tmp14;
    TNode<Object> tmp15;
    TNode<BoolT> tmp16;
    ca_.Bind(&block6, &tmp12, &tmp13, &tmp14, &tmp15, &tmp16);
    TNode<Oddball> tmp17;
    USE(tmp17);
    tmp17 = Null_0(state_);
    TNode<BoolT> tmp18;
    USE(tmp18);
    tmp18 = CodeStubAssembler(state_).TaggedEqual(TNode<Object>{tmp15}, TNode<HeapObject>{tmp17});
    ca_.Goto(&block7, tmp12, tmp13, tmp14, tmp15, tmp16, tmp18);
  }

  if (block7.is_used()) {
    TNode<Context> tmp19;
    TNode<Object> tmp20;
    TNode<Symbol> tmp21;
    TNode<Object> tmp22;
    TNode<BoolT> tmp23;
    TNode<BoolT> tmp24;
    ca_.Bind(&block7, &tmp19, &tmp20, &tmp21, &tmp22, &tmp23, &tmp24);
    ca_.Branch(tmp24, &block3, &block4, tmp19, tmp20, tmp21, tmp22);
  }

  if (block3.is_used()) {
    TNode<Context> tmp25;
    TNode<Object> tmp26;
    TNode<Symbol> tmp27;
    TNode<Object> tmp28;
    ca_.Bind(&block3, &tmp25, &tmp26, &tmp27, &tmp28);
   // ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 1242);
    ca_.Goto(label_IfNullOrUndefined);
  }

  if (block4.is_used()) {
    TNode<Context> tmp29;
    TNode<Object> tmp30;
    TNode<Symbol> tmp31;
    TNode<Object> tmp32;
    ca_.Bind(&block4, &tmp29, &tmp30, &tmp31, &tmp32);
   // ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 1246);
    TNode<JSReceiver> tmp33;
    USE(tmp33);
    compiler::CodeAssemblerLabel label0(&ca_);
    tmp33 = Cast_Callable_1(state_, TNode<Context>{tmp29}, TNode<Object>{tmp32}, &label0);
    ca_.Goto(&block10, tmp29, tmp30, tmp31, tmp32, tmp32, tmp29, tmp33);
    if (label0.is_used()) {
      ca_.Bind(&label0);
      ca_.Goto(&block11, tmp29, tmp30, tmp31, tmp32, tmp32, tmp29);
    }
  }

  if (block11.is_used()) {
    TNode<Context> tmp34;
    TNode<Object> tmp35;
    TNode<Symbol> tmp36;
    TNode<Object> tmp37;
    TNode<Object> tmp38;
    TNode<Context> tmp39;
    ca_.Bind(&block11, &tmp34, &tmp35, &tmp36, &tmp37, &tmp38, &tmp39);
  //  ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 1248);
  //  ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 1247);
    CodeStubAssembler(state_).ThrowTypeError(TNode<Context>{tmp34}, MessageTemplate::kPropertyNotFunction, TNode<Object>{tmp37}, TNode<Object>{tmp36}, TNode<Object>{tmp35});
  }

  if (block10.is_used()) {
    TNode<Context> tmp40;
    TNode<Object> tmp41;
    TNode<Symbol> tmp42;
    TNode<Object> tmp43;
    TNode<Object> tmp44;
    TNode<Context> tmp45;
    TNode<JSReceiver> tmp46;
    ca_.Bind(&block10, &tmp40, &tmp41, &tmp42, &tmp43, &tmp44, &tmp45, &tmp46);
//    ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 1246);
//    ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 1242);
    ca_.Goto(&block12, tmp40, tmp41, tmp42, tmp46);
  }

    TNode<Context> tmp47;
    TNode<Object> tmp48;
    TNode<Symbol> tmp49;
    TNode<JSReceiver> tmp50;
    ca_.Bind(&block12, &tmp47, &tmp48, &tmp49, &tmp50);
  return TNode<JSReceiver>{tmp50};
}


TNode<Object> RequireObjectCoercible_0(compiler::CodeAssemblerState* state_, TNode<Context> p_context, TNode<Object> p_value, const char* p_name) {
  compiler::CodeAssembler ca_(state_);
  compiler::CodeAssemblerParameterizedLabel<Context, Object> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object> block2(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object> block3(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object> block4(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0, p_context, p_value);

  if (block0.is_used()) {
    TNode<Context> tmp0;
    TNode<Object> tmp1;
    ca_.Bind(&block0, &tmp0, &tmp1);
  //  ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 1054);
    TNode<BoolT> tmp2;
    USE(tmp2);
    tmp2 = CodeStubAssembler(state_).IsNullOrUndefined(TNode<Object>{tmp1});
    ca_.Branch(tmp2, &block2, &block3, tmp0, tmp1);
  }

  if (block2.is_used()) {
    TNode<Context> tmp3;
    TNode<Object> tmp4;
    ca_.Bind(&block2, &tmp3, &tmp4);
   // ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 1055);
    CodeStubAssembler(state_).ThrowTypeError(TNode<Context>{tmp3}, MessageTemplate::kCalledOnNullOrUndefined, p_name);
  }

  if (block3.is_used()) {
    TNode<Context> tmp5;
    TNode<Object> tmp6;
    ca_.Bind(&block3, &tmp5, &tmp6);
   // ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 1057);
 //   ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 1051);
    ca_.Goto(&block4, tmp5, tmp6, tmp6);
  }

    TNode<Context> tmp7;
    TNode<Object> tmp8;
    TNode<Object> tmp9;
    ca_.Bind(&block4, &tmp7, &tmp8, &tmp9);
  return TNode<Object>{tmp9};
}

TNode<BoolT> IsFastRegExpStrict_0(compiler::CodeAssemblerState* state_, TNode<Context> p_context, TNode<HeapObject> p_o) {
  compiler::CodeAssembler ca_(state_);
  compiler::CodeAssemblerParameterizedLabel<Context, HeapObject> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, HeapObject, HeapObject> block6(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, HeapObject, HeapObject> block7(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, HeapObject> block5(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, HeapObject> block3(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, HeapObject, BoolT> block1(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, HeapObject, BoolT> block8(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0, p_context, p_o);

  if (block0.is_used()) {
    TNode<Context> tmp0;
    TNode<HeapObject> tmp1;
    ca_.Bind(&block0, &tmp0, &tmp1);
  //  ca_.SetSourcePosition("../../v8/src/builtins/regexp.tq", 13);
    compiler::CodeAssemblerLabel label0(&ca_);
    compiler::CodeAssemblerLabel label1(&ca_);
      RegExpBuiltinsAssembler reba_(state_);
    reba_.BranchIfFastRegExp(TNode<Context>{tmp0}, TNode<HeapObject>{tmp1},reba_.LoadMap(TNode<HeapObject>{tmp1}), &label0, &label1);
    if (label0.is_used()) {
      ca_.Bind(&label0);
      ca_.Goto(&block6, tmp0, tmp1, tmp1);
    }
    if (label1.is_used()) {
      ca_.Bind(&label1);
      ca_.Goto(&block7, tmp0, tmp1, tmp1);
    }
  }

  if (block6.is_used()) {
    TNode<Context> tmp2;
    TNode<HeapObject> tmp3;
    TNode<HeapObject> tmp4;
    ca_.Bind(&block6, &tmp2, &tmp3, &tmp4);
    ca_.Goto(&block5, tmp2, tmp3);
  }

  if (block7.is_used()) {
    TNode<Context> tmp5;
    TNode<HeapObject> tmp6;
    TNode<HeapObject> tmp7;
    ca_.Bind(&block7, &tmp5, &tmp6, &tmp7);
    ca_.Goto(&block3, tmp5, tmp6);
  }

  if (block5.is_used()) {
    TNode<Context> tmp8;
    TNode<HeapObject> tmp9;
    ca_.Bind(&block5, &tmp8, &tmp9);
    TNode<BoolT> tmp10;
    USE(tmp10);
    tmp10 = FromConstexpr_bool_constexpr_bool_0(state_, true);
    ca_.Goto(&block1, tmp8, tmp9, tmp10);
  }

  if (block3.is_used()) {
    TNode<Context> tmp11;
    TNode<HeapObject> tmp12;
    ca_.Bind(&block3, &tmp11, &tmp12);
    TNode<BoolT> tmp13;
    USE(tmp13);
    tmp13 = FromConstexpr_bool_constexpr_bool_0(state_, false);
    ca_.Goto(&block1, tmp11, tmp12, tmp13);
  }

  if (block1.is_used()) {
    TNode<Context> tmp14;
    TNode<HeapObject> tmp15;
    TNode<BoolT> tmp16;
    ca_.Bind(&block1, &tmp14, &tmp15, &tmp16);
//    ca_.SetSourcePosition("../../v8/src/builtins/regexp.tq", 12);
    ca_.Goto(&block8, tmp14, tmp15, tmp16);
  }

    TNode<Context> tmp17;
    TNode<HeapObject> tmp18;
    TNode<BoolT> tmp19;
    ca_.Bind(&block8, &tmp17, &tmp18, &tmp19);
  return TNode<BoolT>{tmp19};
}

TNode<JSRegExp> Cast_FastJSRegExp_0(compiler::CodeAssemblerState* state_, TNode<Context> p_context, TNode<HeapObject> p_o, compiler::CodeAssemblerLabel* label_CastError) {
  compiler::CodeAssembler ca_(state_);
  compiler::CodeAssemblerParameterizedLabel<Context, HeapObject> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, HeapObject> block3(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, HeapObject> block4(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, HeapObject, JSRegExp> block2(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block1(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, HeapObject, JSRegExp> block5(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0, p_context, p_o);

  if (block0.is_used()) {
    TNode<Context> tmp0;
    TNode<HeapObject> tmp1;
    ca_.Bind(&block0, &tmp0, &tmp1);
   // ca_.SetSourcePosition("../../v8/src/builtins/cast.tq", 487);
    TNode<BoolT> tmp2;
    USE(tmp2);
    tmp2 = IsFastRegExpStrict_0(state_, TNode<Context>{tmp0}, TNode<HeapObject>{tmp1});
    ca_.Branch(tmp2, &block3, &block4, tmp0, tmp1);
  }

  if (block3.is_used()) {
    TNode<Context> tmp3;
    TNode<HeapObject> tmp4;
    ca_.Bind(&block3, &tmp3, &tmp4);
   // ca_.SetSourcePosition("../../v8/src/builtins/cast.tq", 488);
    TNode<JSRegExp> tmp5;
    USE(tmp5);
    tmp5 = TORQUE_CAST(TNode<HeapObject>{tmp4});
    ca_.Goto(&block2, tmp3, tmp4, tmp5);
  }

  if (block4.is_used()) {
    TNode<Context> tmp6;
    TNode<HeapObject> tmp7;
    ca_.Bind(&block4, &tmp6, &tmp7);
   // ca_.SetSourcePosition("../../v8/src/builtins/cast.tq", 490);
    ca_.Goto(&block1);
  }

  if (block2.is_used()) {
    TNode<Context> tmp8;
    TNode<HeapObject> tmp9;
    TNode<JSRegExp> tmp10;
    ca_.Bind(&block2, &tmp8, &tmp9, &tmp10);
   // ca_.SetSourcePosition("../../v8/src/builtins/cast.tq", 481);
    ca_.Goto(&block5, tmp8, tmp9, tmp10);
  }

  if (block1.is_used()) {
    ca_.Bind(&block1);
    ca_.Goto(label_CastError);
  }

    TNode<Context> tmp11;
    TNode<HeapObject> tmp12;
    TNode<JSRegExp> tmp13;
    ca_.Bind(&block5, &tmp11, &tmp12, &tmp13);
  return TNode<JSRegExp>{tmp13};
}

TNode<JSRegExp> Cast_FastJSRegExp_1(compiler::CodeAssemblerState* state_, TNode<Context> p_context, TNode<Object> p_o, compiler::CodeAssemblerLabel* label_CastError) {
  compiler::CodeAssembler ca_(state_);
  compiler::CodeAssemblerParameterizedLabel<Context, Object> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object> block4(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, HeapObject> block3(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, HeapObject> block6(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, HeapObject, JSRegExp> block5(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, JSRegExp> block2(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block1(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, JSRegExp> block7(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0, p_context, p_o);

  if (block0.is_used()) {
    TNode<Context> tmp0;
    TNode<Object> tmp1;
    ca_.Bind(&block0, &tmp0, &tmp1);
  //  ca_.SetSourcePosition("../../v8/src/builtins/cast.tq", 96);
    TNode<HeapObject> tmp2;
    USE(tmp2);
    compiler::CodeAssemblerLabel label0(&ca_);
    tmp2 = CodeStubAssembler(state_).TaggedToHeapObject(TNode<Object>{tmp1}, &label0);
    ca_.Goto(&block3, tmp0, tmp1, tmp1, tmp2);
    if (label0.is_used()) {
      ca_.Bind(&label0);
      ca_.Goto(&block4, tmp0, tmp1, tmp1);
    }
  }

  if (block4.is_used()) {
    TNode<Context> tmp3;
    TNode<Object> tmp4;
    TNode<Object> tmp5;
    ca_.Bind(&block4, &tmp3, &tmp4, &tmp5);
    ca_.Goto(&block1);
  }

  if (block3.is_used()) {
    TNode<Context> tmp6;
    TNode<Object> tmp7;
    TNode<Object> tmp8;
    TNode<HeapObject> tmp9;
    ca_.Bind(&block3, &tmp6, &tmp7, &tmp8, &tmp9);
    TNode<JSRegExp> tmp10;
    USE(tmp10);
    compiler::CodeAssemblerLabel label0(&ca_);
    tmp10 = Cast_FastJSRegExp_0(state_, TNode<Context>{tmp6}, TNode<HeapObject>{tmp9}, &label0);
    ca_.Goto(&block5, tmp6, tmp7, tmp9, tmp10);
    if (label0.is_used()) {
      ca_.Bind(&label0);
      ca_.Goto(&block6, tmp6, tmp7, tmp9);
    }
  }

  if (block6.is_used()) {
    TNode<Context> tmp11;
    TNode<Object> tmp12;
    TNode<HeapObject> tmp13;
    ca_.Bind(&block6, &tmp11, &tmp12, &tmp13);
    ca_.Goto(&block1);
  }

  if (block5.is_used()) {
    TNode<Context> tmp14;
    TNode<Object> tmp15;
    TNode<HeapObject> tmp16;
    TNode<JSRegExp> tmp17;
    ca_.Bind(&block5, &tmp14, &tmp15, &tmp16, &tmp17);
    ca_.Goto(&block2, tmp14, tmp15, tmp17);
  }

  if (block2.is_used()) {
    TNode<Context> tmp18;
    TNode<Object> tmp19;
    TNode<JSRegExp> tmp20;
    ca_.Bind(&block2, &tmp18, &tmp19, &tmp20);
//    ca_.SetSourcePosition("../../v8/src/builtins/regexp.tq", 214);
    ca_.Goto(&block7, tmp18, tmp19, tmp20);
  }

  if (block1.is_used()) {
    ca_.Bind(&block1);
    ca_.Goto(label_CastError);
  }

    TNode<Context> tmp21;
    TNode<Object> tmp22;
    TNode<JSRegExp> tmp23;
    ca_.Bind(&block7, &tmp21, &tmp22, &tmp23);
  return TNode<JSRegExp>{tmp23};
}

TNode<String> Cast_DirectString_0(compiler::CodeAssemblerState* state_, TNode<HeapObject> p_o, compiler::CodeAssemblerLabel* label_CastError) {
  compiler::CodeAssembler ca_(state_);
 // compiler::CodeAssembler::SourcePositionScope pos_scope(&ca_);
  compiler::CodeAssemblerParameterizedLabel<> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block4(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block3(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block5(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0);

  TNode<String> tmp0;
  if (block0.is_used()) {
    ca_.Bind(&block0);
   // ca_.SetSourcePosition("../../v8/src/builtins/cast.tq", 438);
    compiler::CodeAssemblerLabel label1(&ca_);
    tmp0 = CodeStubAssembler(state_).TaggedToDirectString(TNode<Object>{p_o}, &label1);
    ca_.Goto(&block3);
    if (label1.is_used()) {
      ca_.Bind(&label1);
      ca_.Goto(&block4);
    }
  }

  if (block4.is_used()) {
    ca_.Bind(&block4);
   // ca_.SetSourcePosition("../../v8/src/builtins/cast.tq", 436);
    ca_.Goto(label_CastError);
  }

  if (block3.is_used()) {
    ca_.Bind(&block3);
    ca_.Goto(&block5);
  }

    ca_.Bind(&block5);
  return TNode<String>{tmp0};
}

TNode<UintPtrT> LoadStringLengthAsUintPtr_0(compiler::CodeAssemblerState* state_, TNode<String> p_s) {
  compiler::CodeAssembler ca_(state_);
  //compiler::CodeAssembler::SourcePositionScope pos_scope(&ca_);
  compiler::CodeAssemblerParameterizedLabel<> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block2(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0);

  TNode<IntPtrT> tmp0;
  TNode<UintPtrT> tmp1;
  if (block0.is_used()) {
    ca_.Bind(&block0);
   // ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 1124);
    tmp0 = CodeStubAssembler(state_).LoadStringLengthAsWord(TNode<String>{p_s});
    tmp1 = CodeStubAssembler(state_).Unsigned(TNode<IntPtrT>{tmp0});
    //ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 1123);
    ca_.Goto(&block2);
  }

    ca_.Bind(&block2);
  return TNode<UintPtrT>{tmp1};
}
TNode<Uint32T> FromConstexpr_uint32_constexpr_int31_0(compiler::CodeAssemblerState* state_, int31_t p_i) {
  compiler::CodeAssembler ca_(state_);
  //compiler::CodeAssembler::SourcePositionScope pos_scope(&ca_);
  compiler::CodeAssemblerParameterizedLabel<> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block2(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0);

  TNode<Int32T> tmp0;
  TNode<Uint32T> tmp1;
  if (block0.is_used()) {
    ca_.Bind(&block0);
  //  ca_.SetSourcePosition("../../v8/src/builtins/convert.tq", 123);
    tmp0 = CodeStubAssembler(state_).Int32Constant(p_i);
    tmp1 = CodeStubAssembler(state_).Unsigned(TNode<Int32T>{tmp0});
    //ca_.SetSourcePosition("../../v8/src/builtins/convert.tq", 122);
    ca_.Goto(&block2);
  }

    ca_.Bind(&block2);
  return TNode<Uint32T>{tmp1};
}
TNode<UintPtrT> FromConstexpr_uintptr_constexpr_int31_0(compiler::CodeAssemblerState* state_, int31_t p_i) {
  compiler::CodeAssembler ca_(state_);
  //compiler::CodeAssembler::SourcePositionScope pos_scope(&ca_);
  compiler::CodeAssemblerParameterizedLabel<> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block2(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0);

  TNode<Uint32T> tmp0;
  TNode<UintPtrT> tmp1;
  if (block0.is_used()) {
    ca_.Bind(&block0);
    //ca_.SetSourcePosition("../../v8/src/builtins/convert.tq", 139);
    tmp0 = FromConstexpr_uint32_constexpr_int31_0(state_, p_i);
    tmp1 = CodeStubAssembler(state_).ChangeUint32ToWord(TNode<Uint32T>{tmp0});
    //ca_.SetSourcePosition("../../v8/src/builtins/convert.tq", 138);
    ca_.Goto(&block2);
  }

    ca_.Bind(&block2);
  return TNode<UintPtrT>{tmp1};
}
TNode<BoolT> FromConstexpr_bool_constexpr_bool_0(compiler::CodeAssemblerState* state_, bool p_b) {
  compiler::CodeAssembler ca_(state_);
  //compiler::CodeAssembler::SourcePositionScope pos_scope(&ca_);
  compiler::CodeAssemblerParameterizedLabel<> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block2(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0);

  TNode<BoolT> tmp0;
  if (block0.is_used()) {
    ca_.Bind(&block0);
   // ca_.SetSourcePosition("../../v8/src/builtins/convert.tq", 151);
    tmp0 = CodeStubAssembler(state_).BoolConstant(p_b);
   // ca_.SetSourcePosition("../../v8/src/builtins/convert.tq", 150);
    ca_.Goto(&block2);
  }

    ca_.Bind(&block2);
  return TNode<BoolT>{tmp0};
}
TNode<Smi> FromConstexpr_Smi_constexpr_int31_0(compiler::CodeAssemblerState* state_, int31_t p_i) {
  compiler::CodeAssembler ca_(state_);
  //compiler::CodeAssembler::SourcePositionScope pos_scope(&ca_);
  compiler::CodeAssemblerParameterizedLabel<> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block2(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0);

  TNode<Smi> tmp0;
  if (block0.is_used()) {
    ca_.Bind(&block0);
    //ca_.SetSourcePosition("../../v8/src/builtins/convert.tq", 82);
    tmp0 = ca_.SmiConstant(CastToUnderlyingTypeIfEnum(p_i));
    //ca_.SetSourcePosition("../../v8/src/builtins/convert.tq", 81);
    ca_.Goto(&block2);
  }

    ca_.Bind(&block2);
  return TNode<Smi>{tmp0};
}
TNode<IntPtrT> FromConstexpr_intptr_constexpr_int31_0(compiler::CodeAssemblerState* state_, int31_t p_i) {
  compiler::CodeAssembler ca_(state_);
  //compiler::CodeAssembler::SourcePositionScope pos_scope(&ca_);
  compiler::CodeAssemblerParameterizedLabel<> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block2(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0);

  TNode<IntPtrT> tmp0;
  if (block0.is_used()) {
    ca_.Bind(&block0);
    //ca_.SetSourcePosition("../../v8/src/builtins/convert.tq", 70);
    tmp0 = ca_.IntPtrConstant(CastToUnderlyingTypeIfEnum(p_i));
    //ca_.SetSourcePosition("../../v8/src/builtins/convert.tq", 69);
    ca_.Goto(&block2);
  }

    ca_.Bind(&block2);
  return TNode<IntPtrT>{tmp0};
}
TNode<Object> FromConstexpr_JSAny_constexpr_string_0(compiler::CodeAssemblerState* state_, const char* p_s) {
  compiler::CodeAssembler ca_(state_);
  //compiler::CodeAssembler::SourcePositionScope pos_scope(&ca_);
  compiler::CodeAssemblerParameterizedLabel<> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block2(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0);

  TNode<String> tmp0;
  if (block0.is_used()) {
    ca_.Bind(&block0);
    //ca_.SetSourcePosition("../../v8/src/builtins/convert.tq", 157);
    tmp0 = CodeStubAssembler(state_).StringConstant(p_s);
    //ca_.SetSourcePosition("../../v8/src/builtins/convert.tq", 156);
    ca_.Goto(&block2);
  }

    ca_.Bind(&block2);
  return TNode<Object>{tmp0};
}
TNode<Smi> TryFastAbstractStringIndexOf_0(compiler::CodeAssemblerState* state_, TNode<Context> p_context, TNode<String> p_string, TNode<String> p_searchString, TNode<Smi> p_fromIndex, compiler::CodeAssemblerLabel* label_Slow) {
  compiler::CodeAssembler ca_(state_);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, UintPtrT, UintPtrT, String> block4(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, UintPtrT, UintPtrT, String, String> block3(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, UintPtrT, UintPtrT, String, String> block6(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, UintPtrT, UintPtrT, String, String, String> block5(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, UintPtrT, UintPtrT, String, String, UintPtrT, UintPtrT> block9(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, UintPtrT, UintPtrT, String, String, UintPtrT, UintPtrT> block7(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, UintPtrT, UintPtrT, String, String, UintPtrT, UintPtrT, UintPtrT, UintPtrT> block13(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, UintPtrT, UintPtrT, String, String, UintPtrT, UintPtrT, UintPtrT, UintPtrT, BoolT> block14(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, UintPtrT, UintPtrT, String, String, UintPtrT, UintPtrT, UintPtrT, UintPtrT, BoolT> block15(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, UintPtrT, UintPtrT, String, String, UintPtrT, UintPtrT, UintPtrT, UintPtrT, BoolT, BoolT> block16(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, UintPtrT, UintPtrT, String, String, UintPtrT, UintPtrT, UintPtrT, UintPtrT, BoolT> block17(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, UintPtrT, UintPtrT, String, String, UintPtrT, UintPtrT, UintPtrT, UintPtrT, BoolT> block18(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, UintPtrT, UintPtrT, String, String, UintPtrT, UintPtrT, UintPtrT, UintPtrT, BoolT, BoolT> block19(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, UintPtrT, UintPtrT, String, String, UintPtrT, UintPtrT, UintPtrT, UintPtrT> block11(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, UintPtrT, UintPtrT, String, String, UintPtrT, UintPtrT, UintPtrT, UintPtrT> block12(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, UintPtrT, UintPtrT, String, String, UintPtrT, UintPtrT, UintPtrT, UintPtrT> block20(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, UintPtrT, UintPtrT, String, String, UintPtrT, UintPtrT, UintPtrT, UintPtrT> block21(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, UintPtrT, UintPtrT, String, String, UintPtrT, UintPtrT> block10(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, UintPtrT, UintPtrT, String, String, UintPtrT, UintPtrT> block8(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, Smi> block2(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<> block1(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, Smi> block22(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0, p_context, p_string, p_searchString, p_fromIndex);

  if (block0.is_used()) {
    TNode<Context> tmp0;
    TNode<String> tmp1;
    TNode<String> tmp2;
    TNode<Smi> tmp3;
    ca_.Bind(&block0, &tmp0, &tmp1, &tmp2, &tmp3);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 22);
    TNode<UintPtrT> tmp4;
    USE(tmp4);
    tmp4 = LoadStringLengthAsUintPtr_0(state_, TNode<String>{tmp1});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 23);
    TNode<UintPtrT> tmp5;
    USE(tmp5);
    tmp5 = LoadStringLengthAsUintPtr_0(state_, TNode<String>{tmp2});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 24);
    TNode<String> tmp6;
    USE(tmp6);
    compiler::CodeAssemblerLabel label0(&ca_);
    tmp6 = Cast_DirectString_0(state_, TNode<HeapObject>{tmp1}, &label0);
    ca_.Goto(&block3, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp1, tmp6);
    if (label0.is_used()) {
      ca_.Bind(&label0);
      ca_.Goto(&block4, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp1);
    }
  }

  if (block4.is_used()) {
    TNode<Context> tmp7;
    TNode<String> tmp8;
    TNode<String> tmp9;
    TNode<Smi> tmp10;
    TNode<UintPtrT> tmp11;
    TNode<UintPtrT> tmp12;
    TNode<String> tmp13;
    ca_.Bind(&block4, &tmp7, &tmp8, &tmp9, &tmp10, &tmp11, &tmp12, &tmp13);
    ca_.Goto(&block1);
  }

  if (block3.is_used()) {
    TNode<Context> tmp14;
    TNode<String> tmp15;
    TNode<String> tmp16;
    TNode<Smi> tmp17;
    TNode<UintPtrT> tmp18;
    TNode<UintPtrT> tmp19;
    TNode<String> tmp20;
    TNode<String> tmp21;
    ca_.Bind(&block3, &tmp14, &tmp15, &tmp16, &tmp17, &tmp18, &tmp19, &tmp20, &tmp21);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 25);
    TNode<String> tmp22;
    USE(tmp22);
    compiler::CodeAssemblerLabel label0(&ca_);
    tmp22 = Cast_DirectString_0(state_, TNode<HeapObject>{tmp16}, &label0);
    ca_.Goto(&block5, tmp14, tmp15, tmp16, tmp17, tmp18, tmp19, tmp21, tmp16, tmp22);
    if (label0.is_used()) {
      ca_.Bind(&label0);
      ca_.Goto(&block6, tmp14, tmp15, tmp16, tmp17, tmp18, tmp19, tmp21, tmp16);
    }
  }

  if (block6.is_used()) {
    TNode<Context> tmp23;
    TNode<String> tmp24;
    TNode<String> tmp25;
    TNode<Smi> tmp26;
    TNode<UintPtrT> tmp27;
    TNode<UintPtrT> tmp28;
    TNode<String> tmp29;
    TNode<String> tmp30;
    ca_.Bind(&block6, &tmp23, &tmp24, &tmp25, &tmp26, &tmp27, &tmp28, &tmp29, &tmp30);
    ca_.Goto(&block1);
  }

  if (block5.is_used()) {
    TNode<Context> tmp31;
    TNode<String> tmp32;
    TNode<String> tmp33;
    TNode<Smi> tmp34;
    TNode<UintPtrT> tmp35;
    TNode<UintPtrT> tmp36;
    TNode<String> tmp37;
    TNode<String> tmp38;
    TNode<String> tmp39;
    ca_.Bind(&block5, &tmp31, &tmp32, &tmp33, &tmp34, &tmp35, &tmp36, &tmp37, &tmp38, &tmp39);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 26);
    TNode<IntPtrT> tmp40;
    USE(tmp40);
    tmp40 = CodeStubAssembler(state_).SmiUntag(TNode<Smi>{tmp34});
    TNode<UintPtrT> tmp41;
    USE(tmp41);
    tmp41 = CodeStubAssembler(state_).Unsigned(TNode<IntPtrT>{tmp40});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 28);
    ca_.Goto(&block9, tmp31, tmp32, tmp33, tmp34, tmp35, tmp36, tmp37, tmp39, tmp41, tmp41);
  }

  if (block9.is_used()) {
    TNode<Context> tmp42;
    TNode<String> tmp43;
    TNode<String> tmp44;
    TNode<Smi> tmp45;
    TNode<UintPtrT> tmp46;
    TNode<UintPtrT> tmp47;
    TNode<String> tmp48;
    TNode<String> tmp49;
    TNode<UintPtrT> tmp50;
    TNode<UintPtrT> tmp51;
    ca_.Bind(&block9, &tmp42, &tmp43, &tmp44, &tmp45, &tmp46, &tmp47, &tmp48, &tmp49, &tmp50, &tmp51);
    TNode<BoolT> tmp52;
    USE(tmp52);
    tmp52 = CodeStubAssembler(state_).UintPtrLessThan(TNode<UintPtrT>{tmp51}, TNode<UintPtrT>{tmp46});
    ca_.Branch(tmp52, &block7, &block8, tmp42, tmp43, tmp44, tmp45, tmp46, tmp47, tmp48, tmp49, tmp50, tmp51);
  }

  if (block7.is_used()) {
    TNode<Context> tmp53;
    TNode<String> tmp54;
    TNode<String> tmp55;
    TNode<Smi> tmp56;
    TNode<UintPtrT> tmp57;
    TNode<UintPtrT> tmp58;
    TNode<String> tmp59;
    TNode<String> tmp60;
    TNode<UintPtrT> tmp61;
    TNode<UintPtrT> tmp62;
    ca_.Bind(&block7, &tmp53, &tmp54, &tmp55, &tmp56, &tmp57, &tmp58, &tmp59, &tmp60, &tmp61, &tmp62);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 29);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 30);
    TNode<UintPtrT> tmp63;
    USE(tmp63);
    tmp63 = FromConstexpr_uintptr_constexpr_int31_0(state_, 0);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 31);
    ca_.Goto(&block13, tmp53, tmp54, tmp55, tmp56, tmp57, tmp58, tmp59, tmp60, tmp61, tmp62, tmp62, tmp63);
  }

  if (block13.is_used()) {
    TNode<Context> tmp64;
    TNode<String> tmp65;
    TNode<String> tmp66;
    TNode<Smi> tmp67;
    TNode<UintPtrT> tmp68;
    TNode<UintPtrT> tmp69;
    TNode<String> tmp70;
    TNode<String> tmp71;
    TNode<UintPtrT> tmp72;
    TNode<UintPtrT> tmp73;
    TNode<UintPtrT> tmp74;
    TNode<UintPtrT> tmp75;
    ca_.Bind(&block13, &tmp64, &tmp65, &tmp66, &tmp67, &tmp68, &tmp69, &tmp70, &tmp71, &tmp72, &tmp73, &tmp74, &tmp75);
    TNode<BoolT> tmp76;
    USE(tmp76);
    tmp76 = CodeStubAssembler(state_).UintPtrLessThan(TNode<UintPtrT>{tmp74}, TNode<UintPtrT>{tmp68});
    ca_.Branch(tmp76, &block14, &block15, tmp64, tmp65, tmp66, tmp67, tmp68, tmp69, tmp70, tmp71, tmp72, tmp73, tmp74, tmp75, tmp76);
  }

  if (block14.is_used()) {
    TNode<Context> tmp77;
    TNode<String> tmp78;
    TNode<String> tmp79;
    TNode<Smi> tmp80;
    TNode<UintPtrT> tmp81;
    TNode<UintPtrT> tmp82;
    TNode<String> tmp83;
    TNode<String> tmp84;
    TNode<UintPtrT> tmp85;
    TNode<UintPtrT> tmp86;
    TNode<UintPtrT> tmp87;
    TNode<UintPtrT> tmp88;
    TNode<BoolT> tmp89;
    ca_.Bind(&block14, &tmp77, &tmp78, &tmp79, &tmp80, &tmp81, &tmp82, &tmp83, &tmp84, &tmp85, &tmp86, &tmp87, &tmp88, &tmp89);
    TNode<BoolT> tmp90;
    USE(tmp90);
    tmp90 = CodeStubAssembler(state_).UintPtrLessThan(TNode<UintPtrT>{tmp88}, TNode<UintPtrT>{tmp82});
    ca_.Goto(&block16, tmp77, tmp78, tmp79, tmp80, tmp81, tmp82, tmp83, tmp84, tmp85, tmp86, tmp87, tmp88, tmp89, tmp90);
  }

  if (block15.is_used()) {
    TNode<Context> tmp91;
    TNode<String> tmp92;
    TNode<String> tmp93;
    TNode<Smi> tmp94;
    TNode<UintPtrT> tmp95;
    TNode<UintPtrT> tmp96;
    TNode<String> tmp97;
    TNode<String> tmp98;
    TNode<UintPtrT> tmp99;
    TNode<UintPtrT> tmp100;
    TNode<UintPtrT> tmp101;
    TNode<UintPtrT> tmp102;
    TNode<BoolT> tmp103;
    ca_.Bind(&block15, &tmp91, &tmp92, &tmp93, &tmp94, &tmp95, &tmp96, &tmp97, &tmp98, &tmp99, &tmp100, &tmp101, &tmp102, &tmp103);
    TNode<BoolT> tmp104;
    USE(tmp104);
    tmp104 = FromConstexpr_bool_constexpr_bool_0(state_, false);
    ca_.Goto(&block16, tmp91, tmp92, tmp93, tmp94, tmp95, tmp96, tmp97, tmp98, tmp99, tmp100, tmp101, tmp102, tmp103, tmp104);
  }

  if (block16.is_used()) {
    TNode<Context> tmp105;
    TNode<String> tmp106;
    TNode<String> tmp107;
    TNode<Smi> tmp108;
    TNode<UintPtrT> tmp109;
    TNode<UintPtrT> tmp110;
    TNode<String> tmp111;
    TNode<String> tmp112;
    TNode<UintPtrT> tmp113;
    TNode<UintPtrT> tmp114;
    TNode<UintPtrT> tmp115;
    TNode<UintPtrT> tmp116;
    TNode<BoolT> tmp117;
    TNode<BoolT> tmp118;
    ca_.Bind(&block16, &tmp105, &tmp106, &tmp107, &tmp108, &tmp109, &tmp110, &tmp111, &tmp112, &tmp113, &tmp114, &tmp115, &tmp116, &tmp117, &tmp118);
    ca_.Branch(tmp118, &block17, &block18, tmp105, tmp106, tmp107, tmp108, tmp109, tmp110, tmp111, tmp112, tmp113, tmp114, tmp115, tmp116, tmp118);
  }

  if (block17.is_used()) {
    TNode<Context> tmp119;
    TNode<String> tmp120;
    TNode<String> tmp121;
    TNode<Smi> tmp122;
    TNode<UintPtrT> tmp123;
    TNode<UintPtrT> tmp124;
    TNode<String> tmp125;
    TNode<String> tmp126;
    TNode<UintPtrT> tmp127;
    TNode<UintPtrT> tmp128;
    TNode<UintPtrT> tmp129;
    TNode<UintPtrT> tmp130;
    TNode<BoolT> tmp131;
    ca_.Bind(&block17, &tmp119, &tmp120, &tmp121, &tmp122, &tmp123, &tmp124, &tmp125, &tmp126, &tmp127, &tmp128, &tmp129, &tmp130, &tmp131);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 32);
    TNode<Int32T> tmp132;
    USE(tmp132);
    tmp132 = CodeStubAssembler(state_).StringCharCodeAt(TNode<String>{tmp125},CodeStubAssembler(state_).Signed(TNode<UintPtrT>{tmp129}));
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 33);
    TNode<Int32T> tmp133;
    USE(tmp133);
    tmp133 = CodeStubAssembler(state_).StringCharCodeAt(TNode<String>{tmp126},CodeStubAssembler(state_).Signed(TNode<UintPtrT>{tmp130}));
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 32);
    TNode<BoolT> tmp134;
    USE(tmp134);
    tmp134 = CodeStubAssembler(state_).Word32Equal(TNode<Int32T>{tmp132}, TNode<Int32T>{tmp133});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 31);
    ca_.Goto(&block19, tmp119, tmp120, tmp121, tmp122, tmp123, tmp124, tmp125, tmp126, tmp127, tmp128, tmp129, tmp130, tmp131, tmp134);
  }

  if (block18.is_used()) {
    TNode<Context> tmp135;
    TNode<String> tmp136;
    TNode<String> tmp137;
    TNode<Smi> tmp138;
    TNode<UintPtrT> tmp139;
    TNode<UintPtrT> tmp140;
    TNode<String> tmp141;
    TNode<String> tmp142;
    TNode<UintPtrT> tmp143;
    TNode<UintPtrT> tmp144;
    TNode<UintPtrT> tmp145;
    TNode<UintPtrT> tmp146;
    TNode<BoolT> tmp147;
    ca_.Bind(&block18, &tmp135, &tmp136, &tmp137, &tmp138, &tmp139, &tmp140, &tmp141, &tmp142, &tmp143, &tmp144, &tmp145, &tmp146, &tmp147);
    TNode<BoolT> tmp148;
    USE(tmp148);
    tmp148 = FromConstexpr_bool_constexpr_bool_0(state_, false);
    ca_.Goto(&block19, tmp135, tmp136, tmp137, tmp138, tmp139, tmp140, tmp141, tmp142, tmp143, tmp144, tmp145, tmp146, tmp147, tmp148);
  }

  if (block19.is_used()) {
    TNode<Context> tmp149;
    TNode<String> tmp150;
    TNode<String> tmp151;
    TNode<Smi> tmp152;
    TNode<UintPtrT> tmp153;
    TNode<UintPtrT> tmp154;
    TNode<String> tmp155;
    TNode<String> tmp156;
    TNode<UintPtrT> tmp157;
    TNode<UintPtrT> tmp158;
    TNode<UintPtrT> tmp159;
    TNode<UintPtrT> tmp160;
    TNode<BoolT> tmp161;
    TNode<BoolT> tmp162;
    ca_.Bind(&block19, &tmp149, &tmp150, &tmp151, &tmp152, &tmp153, &tmp154, &tmp155, &tmp156, &tmp157, &tmp158, &tmp159, &tmp160, &tmp161, &tmp162);
    ca_.Branch(tmp162, &block11, &block12, tmp149, tmp150, tmp151, tmp152, tmp153, tmp154, tmp155, tmp156, tmp157, tmp158, tmp159, tmp160);
  }

  if (block11.is_used()) {
    TNode<Context> tmp163;
    TNode<String> tmp164;
    TNode<String> tmp165;
    TNode<Smi> tmp166;
    TNode<UintPtrT> tmp167;
    TNode<UintPtrT> tmp168;
    TNode<String> tmp169;
    TNode<String> tmp170;
    TNode<UintPtrT> tmp171;
    TNode<UintPtrT> tmp172;
    TNode<UintPtrT> tmp173;
    TNode<UintPtrT> tmp174;
    ca_.Bind(&block11, &tmp163, &tmp164, &tmp165, &tmp166, &tmp167, &tmp168, &tmp169, &tmp170, &tmp171, &tmp172, &tmp173, &tmp174);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 34);
    TNode<UintPtrT> tmp175;
    USE(tmp175);
    tmp175 = FromConstexpr_uintptr_constexpr_int31_0(state_, 1);
    TNode<UintPtrT> tmp176;
    USE(tmp176);
    tmp176 = CodeStubAssembler(state_).UintPtrAdd(TNode<UintPtrT>{tmp173}, TNode<UintPtrT>{tmp175});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 35);
    TNode<UintPtrT> tmp177;
    USE(tmp177);
    tmp177 = FromConstexpr_uintptr_constexpr_int31_0(state_, 1);
    TNode<UintPtrT> tmp178;
    USE(tmp178);
    tmp178 = CodeStubAssembler(state_).UintPtrAdd(TNode<UintPtrT>{tmp174}, TNode<UintPtrT>{tmp177});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 31);
    ca_.Goto(&block13, tmp163, tmp164, tmp165, tmp166, tmp167, tmp168, tmp169, tmp170, tmp171, tmp172, tmp176, tmp178);
  }

  if (block12.is_used()) {
    TNode<Context> tmp179;
    TNode<String> tmp180;
    TNode<String> tmp181;
    TNode<Smi> tmp182;
    TNode<UintPtrT> tmp183;
    TNode<UintPtrT> tmp184;
    TNode<String> tmp185;
    TNode<String> tmp186;
    TNode<UintPtrT> tmp187;
    TNode<UintPtrT> tmp188;
    TNode<UintPtrT> tmp189;
    TNode<UintPtrT> tmp190;
    ca_.Bind(&block12, &tmp179, &tmp180, &tmp181, &tmp182, &tmp183, &tmp184, &tmp185, &tmp186, &tmp187, &tmp188, &tmp189, &tmp190);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 37);
    TNode<BoolT> tmp191;
    USE(tmp191);
    tmp191 = CodeStubAssembler(state_).WordEqual(TNode<UintPtrT>{tmp190}, TNode<UintPtrT>{tmp184});
    ca_.Branch(tmp191, &block20, &block21, tmp179, tmp180, tmp181, tmp182, tmp183, tmp184, tmp185, tmp186, tmp187, tmp188, tmp189, tmp190);
  }

  if (block20.is_used()) {
    TNode<Context> tmp192;
    TNode<String> tmp193;
    TNode<String> tmp194;
    TNode<Smi> tmp195;
    TNode<UintPtrT> tmp196;
    TNode<UintPtrT> tmp197;
    TNode<String> tmp198;
    TNode<String> tmp199;
    TNode<UintPtrT> tmp200;
    TNode<UintPtrT> tmp201;
    TNode<UintPtrT> tmp202;
    TNode<UintPtrT> tmp203;
    ca_.Bind(&block20, &tmp192, &tmp193, &tmp194, &tmp195, &tmp196, &tmp197, &tmp198, &tmp199, &tmp200, &tmp201, &tmp202, &tmp203);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 38);
    TNode<IntPtrT> tmp204;
    USE(tmp204);
    tmp204 = CodeStubAssembler(state_).Signed(TNode<UintPtrT>{tmp201});
    TNode<Smi> tmp205;
    USE(tmp205);
    tmp205 = CodeStubAssembler(state_).SmiTag(TNode<IntPtrT>{tmp204});
    ca_.Goto(&block2, tmp192, tmp193, tmp194, tmp195, tmp205);
  }

  if (block21.is_used()) {
    TNode<Context> tmp206;
    TNode<String> tmp207;
    TNode<String> tmp208;
    TNode<Smi> tmp209;
    TNode<UintPtrT> tmp210;
    TNode<UintPtrT> tmp211;
    TNode<String> tmp212;
    TNode<String> tmp213;
    TNode<UintPtrT> tmp214;
    TNode<UintPtrT> tmp215;
    TNode<UintPtrT> tmp216;
    TNode<UintPtrT> tmp217;
    ca_.Bind(&block21, &tmp206, &tmp207, &tmp208, &tmp209, &tmp210, &tmp211, &tmp212, &tmp213, &tmp214, &tmp215, &tmp216, &tmp217);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 28);
    ca_.Goto(&block10, tmp206, tmp207, tmp208, tmp209, tmp210, tmp211, tmp212, tmp213, tmp214, tmp215);
  }

  if (block10.is_used()) {
    TNode<Context> tmp218;
    TNode<String> tmp219;
    TNode<String> tmp220;
    TNode<Smi> tmp221;
    TNode<UintPtrT> tmp222;
    TNode<UintPtrT> tmp223;
    TNode<String> tmp224;
    TNode<String> tmp225;
    TNode<UintPtrT> tmp226;
    TNode<UintPtrT> tmp227;
    ca_.Bind(&block10, &tmp218, &tmp219, &tmp220, &tmp221, &tmp222, &tmp223, &tmp224, &tmp225, &tmp226, &tmp227);
    TNode<UintPtrT> tmp228;
    USE(tmp228);
    tmp228 = FromConstexpr_uintptr_constexpr_int31_0(state_, 1);
    TNode<UintPtrT> tmp229;
    USE(tmp229);
    tmp229 = CodeStubAssembler(state_).UintPtrAdd(TNode<UintPtrT>{tmp227}, TNode<UintPtrT>{tmp228});
    ca_.Goto(&block9, tmp218, tmp219, tmp220, tmp221, tmp222, tmp223, tmp224, tmp225, tmp226, tmp229);
  }

  if (block8.is_used()) {
    TNode<Context> tmp230;
    TNode<String> tmp231;
    TNode<String> tmp232;
    TNode<Smi> tmp233;
    TNode<UintPtrT> tmp234;
    TNode<UintPtrT> tmp235;
    TNode<String> tmp236;
    TNode<String> tmp237;
    TNode<UintPtrT> tmp238;
    TNode<UintPtrT> tmp239;
    ca_.Bind(&block8, &tmp230, &tmp231, &tmp232, &tmp233, &tmp234, &tmp235, &tmp236, &tmp237, &tmp238, &tmp239);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 41);
    TNode<Smi> tmp240;
    USE(tmp240);
    tmp240 = FromConstexpr_Smi_constexpr_int31_0(state_, -1);
    ca_.Goto(&block2, tmp230, tmp231, tmp232, tmp233, tmp240);
  }

  if (block2.is_used()) {
    TNode<Context> tmp241;
    TNode<String> tmp242;
    TNode<String> tmp243;
    TNode<Smi> tmp244;
    TNode<Smi> tmp245;
    ca_.Bind(&block2, &tmp241, &tmp242, &tmp243, &tmp244, &tmp245);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 20);
    ca_.Goto(&block22, tmp241, tmp242, tmp243, tmp244, tmp245);
  }

  if (block1.is_used()) {
    ca_.Bind(&block1);
    ca_.Goto(label_Slow);
  }

    TNode<Context> tmp246;
    TNode<String> tmp247;
    TNode<String> tmp248;
    TNode<Smi> tmp249;
    TNode<Smi> tmp250;
    ca_.Bind(&block22, &tmp246, &tmp247, &tmp248, &tmp249, &tmp250);
  return TNode<Smi>{tmp250};
}

TNode<Smi> AbstractStringIndexOf_0(compiler::CodeAssemblerState* state_, TNode<Context> p_context, TNode<String> p_string, TNode<String> p_searchString, TNode<Smi> p_fromIndex) {
  compiler::CodeAssembler ca_(state_);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, IntPtrT, IntPtrT, BoolT> block4(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, IntPtrT, IntPtrT, BoolT> block5(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, IntPtrT, IntPtrT, BoolT, BoolT> block6(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, IntPtrT, IntPtrT> block2(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, IntPtrT, IntPtrT> block3(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, IntPtrT, IntPtrT> block7(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, IntPtrT, IntPtrT> block8(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, IntPtrT, IntPtrT, String, String, Smi, Context> block12(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, IntPtrT, IntPtrT, String, String, Smi, Context, Smi> block11(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, IntPtrT, IntPtrT, IntPtrT> block15(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, IntPtrT, IntPtrT, IntPtrT> block13(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, IntPtrT, IntPtrT, IntPtrT> block17(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, IntPtrT, IntPtrT, IntPtrT> block18(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, IntPtrT, IntPtrT, IntPtrT> block14(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, Smi> block1(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, String, String, Smi, Smi> block19(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0, p_context, p_string, p_searchString, p_fromIndex);

  if (block0.is_used()) {
    TNode<Context> tmp0;
    TNode<String> tmp1;
    TNode<String> tmp2;
    TNode<Smi> tmp3;
    ca_.Bind(&block0, &tmp0, &tmp1, &tmp2, &tmp3);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 43);
    TNode<IntPtrT> tmp4;
    USE(tmp4);
    tmp4 = CodeStubAssembler(state_).LoadStringLengthAsWord(TNode<String>{tmp2});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 44);
    TNode<IntPtrT> tmp5;
    USE(tmp5);
    tmp5 = CodeStubAssembler(state_).LoadStringLengthAsWord(TNode<String>{tmp1});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 45);
    TNode<IntPtrT> tmp6;
    USE(tmp6);
    tmp6 = FromConstexpr_intptr_constexpr_int31_0(state_, 0);
    TNode<BoolT> tmp7;
    USE(tmp7);
    tmp7 = CodeStubAssembler(state_).WordEqual(TNode<IntPtrT>{tmp4}, TNode<IntPtrT>{tmp6});
    ca_.Branch(tmp7, &block4, &block5, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp7);
  }

  if (block4.is_used()) {
    TNode<Context> tmp8;
    TNode<String> tmp9;
    TNode<String> tmp10;
    TNode<Smi> tmp11;
    TNode<IntPtrT> tmp12;
    TNode<IntPtrT> tmp13;
    TNode<BoolT> tmp14;
    ca_.Bind(&block4, &tmp8, &tmp9, &tmp10, &tmp11, &tmp12, &tmp13, &tmp14);
    TNode<IntPtrT> tmp15;
    USE(tmp15);
    tmp15 = CodeStubAssembler(state_).SmiUntag(TNode<Smi>{tmp11});
    TNode<BoolT> tmp16;
    USE(tmp16);
    tmp16 = CodeStubAssembler(state_).IntPtrLessThanOrEqual(TNode<IntPtrT>{tmp15}, TNode<IntPtrT>{tmp13});
    ca_.Goto(&block6, tmp8, tmp9, tmp10, tmp11, tmp12, tmp13, tmp14, tmp16);
  }

  if (block5.is_used()) {
    TNode<Context> tmp17;
    TNode<String> tmp18;
    TNode<String> tmp19;
    TNode<Smi> tmp20;
    TNode<IntPtrT> tmp21;
    TNode<IntPtrT> tmp22;
    TNode<BoolT> tmp23;
    ca_.Bind(&block5, &tmp17, &tmp18, &tmp19, &tmp20, &tmp21, &tmp22, &tmp23);
    TNode<BoolT> tmp24;
    USE(tmp24);
    tmp24 = FromConstexpr_bool_constexpr_bool_0(state_, false);
    ca_.Goto(&block6, tmp17, tmp18, tmp19, tmp20, tmp21, tmp22, tmp23, tmp24);
  }

  if (block6.is_used()) {
    TNode<Context> tmp25;
    TNode<String> tmp26;
    TNode<String> tmp27;
    TNode<Smi> tmp28;
    TNode<IntPtrT> tmp29;
    TNode<IntPtrT> tmp30;
    TNode<BoolT> tmp31;
    TNode<BoolT> tmp32;
    ca_.Bind(&block6, &tmp25, &tmp26, &tmp27, &tmp28, &tmp29, &tmp30, &tmp31, &tmp32);
    ca_.Branch(tmp32, &block2, &block3, tmp25, tmp26, tmp27, tmp28, tmp29, tmp30);
  }

  if (block2.is_used()) {
    TNode<Context> tmp33;
    TNode<String> tmp34;
    TNode<String> tmp35;
    TNode<Smi> tmp36;
    TNode<IntPtrT> tmp37;
    TNode<IntPtrT> tmp38;
    ca_.Bind(&block2, &tmp33, &tmp34, &tmp35, &tmp36, &tmp37, &tmp38);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 46);
    ca_.Goto(&block1, tmp33, tmp34, tmp35, tmp36, tmp36);
  }

  if (block3.is_used()) {
    TNode<Context> tmp39;
    TNode<String> tmp40;
    TNode<String> tmp41;
    TNode<Smi> tmp42;
    TNode<IntPtrT> tmp43;
    TNode<IntPtrT> tmp44;
    ca_.Bind(&block3, &tmp39, &tmp40, &tmp41, &tmp42, &tmp43, &tmp44);
   // ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 52);
    TNode<IntPtrT> tmp45;
    USE(tmp45);
    tmp45 = CodeStubAssembler(state_).SmiUntag(TNode<Smi>{tmp42});
    TNode<IntPtrT> tmp46;
    USE(tmp46);
    tmp46 = CodeStubAssembler(state_).IntPtrAdd(TNode<IntPtrT>{tmp45}, TNode<IntPtrT>{tmp43});
    TNode<BoolT> tmp47;
    USE(tmp47);
    tmp47 = CodeStubAssembler(state_).IntPtrGreaterThan(TNode<IntPtrT>{tmp46}, TNode<IntPtrT>{tmp44});
    ca_.Branch(tmp47, &block7, &block8, tmp39, tmp40, tmp41, tmp42, tmp43, tmp44);
  }

  if (block7.is_used()) {
    TNode<Context> tmp48;
    TNode<String> tmp49;
    TNode<String> tmp50;
    TNode<Smi> tmp51;
    TNode<IntPtrT> tmp52;
    TNode<IntPtrT> tmp53;
    ca_.Bind(&block7, &tmp48, &tmp49, &tmp50, &tmp51, &tmp52, &tmp53);
   // ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 53);
    TNode<Smi> tmp54;
    USE(tmp54);
    tmp54 = FromConstexpr_Smi_constexpr_int31_0(state_, -1);
    ca_.Goto(&block1, tmp48, tmp49, tmp50, tmp51, tmp54);
  }

  if (block8.is_used()) {
    TNode<Context> tmp55;
    TNode<String> tmp56;
    TNode<String> tmp57;
    TNode<Smi> tmp58;
    TNode<IntPtrT> tmp59;
    TNode<IntPtrT> tmp60;
    ca_.Bind(&block8, &tmp55, &tmp56, &tmp57, &tmp58, &tmp59, &tmp60);
   // ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 57);
    TNode<Smi> tmp61;
    USE(tmp61);
    compiler::CodeAssemblerLabel label0(&ca_);
    tmp61 = TryFastAbstractStringIndexOf_0(state_, TNode<Context>{tmp55}, TNode<String>{tmp56}, TNode<String>{tmp57}, TNode<Smi>{tmp58}, &label0);
    ca_.Goto(&block11, tmp55, tmp56, tmp57, tmp58, tmp59, tmp60, tmp56, tmp57, tmp58, tmp55, tmp61);
    if (label0.is_used()) {
      ca_.Bind(&label0);
      ca_.Goto(&block12, tmp55, tmp56, tmp57, tmp58, tmp59, tmp60, tmp56, tmp57, tmp58, tmp55);
    }
  }

  if (block12.is_used()) {
    TNode<Context> tmp62;
    TNode<String> tmp63;
    TNode<String> tmp64;
    TNode<Smi> tmp65;
    TNode<IntPtrT> tmp66;
    TNode<IntPtrT> tmp67;
    TNode<String> tmp68;
    TNode<String> tmp69;
    TNode<Smi> tmp70;
    TNode<Context> tmp71;
    ca_.Bind(&block12, &tmp62, &tmp63, &tmp64, &tmp65, &tmp66, &tmp67, &tmp68, &tmp69, &tmp70, &tmp71);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 61);
    TNode<IntPtrT> tmp72;
    USE(tmp72);
    tmp72 = CodeStubAssembler(state_).SmiUntag(TNode<Smi>{tmp65});
    ca_.Goto(&block15, tmp62, tmp63, tmp64, tmp65, tmp66, tmp67, tmp72);
  }

  if (block11.is_used()) {
    TNode<Context> tmp73;
    TNode<String> tmp74;
    TNode<String> tmp75;
    TNode<Smi> tmp76;
    TNode<IntPtrT> tmp77;
    TNode<IntPtrT> tmp78;
    TNode<String> tmp79;
    TNode<String> tmp80;
    TNode<Smi> tmp81;
    TNode<Context> tmp82;
    TNode<Smi> tmp83;
    ca_.Bind(&block11, &tmp73, &tmp74, &tmp75, &tmp76, &tmp77, &tmp78, &tmp79, &tmp80, &tmp81, &tmp82, &tmp83);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 57);
    ca_.Goto(&block1, tmp73, tmp74, tmp75, tmp76, tmp83);
  }

  if (block15.is_used()) {
    TNode<Context> tmp84;
    TNode<String> tmp85;
    TNode<String> tmp86;
    TNode<Smi> tmp87;
    TNode<IntPtrT> tmp88;
    TNode<IntPtrT> tmp89;
    TNode<IntPtrT> tmp90;
    ca_.Bind(&block15, &tmp84, &tmp85, &tmp86, &tmp87, &tmp88, &tmp89, &tmp90);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 62);
    TNode<IntPtrT> tmp91;
    USE(tmp91);
    tmp91 = CodeStubAssembler(state_).IntPtrAdd(TNode<IntPtrT>{tmp90}, TNode<IntPtrT>{tmp88});
    TNode<BoolT> tmp92;
    USE(tmp92);
    tmp92 = CodeStubAssembler(state_).IntPtrLessThanOrEqual(TNode<IntPtrT>{tmp91}, TNode<IntPtrT>{tmp89});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 61);
    ca_.Branch(tmp92, &block13, &block14, tmp84, tmp85, tmp86, tmp87, tmp88, tmp89, tmp90);
  }

  if (block13.is_used()) {
    TNode<Context> tmp93;
    TNode<String> tmp94;
    TNode<String> tmp95;
    TNode<Smi> tmp96;
    TNode<IntPtrT> tmp97;
    TNode<IntPtrT> tmp98;
    TNode<IntPtrT> tmp99;
    ca_.Bind(&block13, &tmp93, &tmp94, &tmp95, &tmp96, &tmp97, &tmp98, &tmp99);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 64);
    TNode<Smi> tmp100;
    USE(tmp100);
    tmp100 = CodeStubAssembler(state_).SmiTag(TNode<IntPtrT>{tmp99});
    TNode<Number> tmp101;
    USE(tmp101);
    tmp101 = Convert_Number_Smi_0(state_, TNode<Smi>{tmp100});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 63);
    TNode<Oddball> tmp102;
    tmp102 = TORQUE_CAST(CodeStubAssembler(state_).CallRuntime(Runtime::kStringCompareSequence, tmp93, tmp94, tmp95, tmp101)); 
    USE(tmp102);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 65);
    TNode<Oddball> tmp103;
    USE(tmp103);
    tmp103 = True_0(state_);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 63);
    TNode<BoolT> tmp104;
    USE(tmp104);
    tmp104 = CodeStubAssembler(state_).TaggedEqual(TNode<HeapObject>{tmp102}, TNode<HeapObject>{tmp103});
    ca_.Branch(tmp104, &block17, &block18, tmp93, tmp94, tmp95, tmp96, tmp97, tmp98, tmp99);
  }

  if (block17.is_used()) {
    TNode<Context> tmp105;
    TNode<String> tmp106;
    TNode<String> tmp107;
    TNode<Smi> tmp108;
    TNode<IntPtrT> tmp109;
    TNode<IntPtrT> tmp110;
    TNode<IntPtrT> tmp111;
    ca_.Bind(&block17, &tmp105, &tmp106, &tmp107, &tmp108, &tmp109, &tmp110, &tmp111);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 66);
    TNode<Smi> tmp112;
    USE(tmp112);
    tmp112 = CodeStubAssembler(state_).SmiTag(TNode<IntPtrT>{tmp111});
    ca_.Goto(&block1, tmp105, tmp106, tmp107, tmp108, tmp112);
  }

  if (block18.is_used()) {
    TNode<Context> tmp113;
    TNode<String> tmp114;
    TNode<String> tmp115;
    TNode<Smi> tmp116;
    TNode<IntPtrT> tmp117;
    TNode<IntPtrT> tmp118;
    TNode<IntPtrT> tmp119;
    ca_.Bind(&block18, &tmp113, &tmp114, &tmp115, &tmp116, &tmp117, &tmp118, &tmp119);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 62);
    TNode<IntPtrT> tmp120;
    USE(tmp120);
    tmp120 = FromConstexpr_intptr_constexpr_int31_0(state_, 1);
    TNode<IntPtrT> tmp121;
    USE(tmp121);
    tmp121 = CodeStubAssembler(state_).IntPtrAdd(TNode<IntPtrT>{tmp119}, TNode<IntPtrT>{tmp120});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 61);
    ca_.Goto(&block15, tmp113, tmp114, tmp115, tmp116, tmp117, tmp118, tmp121);
  }

  if (block14.is_used()) {
    TNode<Context> tmp122;
    TNode<String> tmp123;
    TNode<String> tmp124;
    TNode<Smi> tmp125;
    TNode<IntPtrT> tmp126;
    TNode<IntPtrT> tmp127;
    TNode<IntPtrT> tmp128;
    ca_.Bind(&block14, &tmp122, &tmp123, &tmp124, &tmp125, &tmp126, &tmp127, &tmp128);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 69);
    TNode<Smi> tmp129;
    USE(tmp129);
    tmp129 = FromConstexpr_Smi_constexpr_int31_0(state_, -1);
    ca_.Goto(&block1, tmp122, tmp123, tmp124, tmp125, tmp129);
  }

  if (block1.is_used()) {
    TNode<Context> tmp130;
    TNode<String> tmp131;
    TNode<String> tmp132;
    TNode<Smi> tmp133;
    TNode<Smi> tmp134;
    ca_.Bind(&block1, &tmp130, &tmp131, &tmp132, &tmp133, &tmp134);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 40);
    ca_.Goto(&block19, tmp130, tmp131, tmp132, tmp133, tmp134);
  }

    TNode<Context> tmp135;
    TNode<String> tmp136;
    TNode<String> tmp137;
    TNode<Smi> tmp138;
    TNode<Smi> tmp139;
    ca_.Bind(&block19, &tmp135, &tmp136, &tmp137, &tmp138, &tmp139);
  return TNode<Smi>{tmp139};
}

void ThrowIfNotGlobal_0(compiler::CodeAssemblerState* state_, TNode<Context> p_context, TNode<Object> p_searchValue) {
  compiler::CodeAssembler ca_(state_);
  compiler::CodeAssemblerParameterizedLabel<Context, Object> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, BoolT, Object, Object> block5(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, BoolT, Object, Object, JSRegExp> block4(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, BoolT, Object> block3(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, BoolT, Object> block2(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, BoolT> block6(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, BoolT> block7(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object> block1(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object> block8(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0, p_context, p_searchValue);

  if (block0.is_used()) {
    TNode<Context> tmp0;
    TNode<Object> tmp1;
    ca_.Bind(&block0, &tmp0, &tmp1);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 70);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 71);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 72);
    TNode<JSRegExp> tmp2;
    USE(tmp2);
    compiler::CodeAssemblerLabel label0(&ca_);
    tmp2 = Cast_FastJSRegExp_1(state_, TNode<Context>{tmp0}, TNode<Object>{tmp1}, &label0);
    ca_.Goto(&block4, tmp0, tmp1, ca_.Uninitialized<BoolT>(), tmp1, tmp1, tmp2);
    if (label0.is_used()) {
      ca_.Bind(&label0);
      ca_.Goto(&block5, tmp0, tmp1, ca_.Uninitialized<BoolT>(), tmp1, tmp1);
    }
  }

  if (block5.is_used()) {
    TNode<Context> tmp3;
    TNode<Object> tmp4;
    TNode<BoolT> tmp5;
    TNode<Object> tmp6;
    TNode<Object> tmp7;
    ca_.Bind(&block5, &tmp3, &tmp4, &tmp5, &tmp6, &tmp7);
    ca_.Goto(&block3, tmp3, tmp4, tmp5, tmp6);
  }

  if (block4.is_used()) {
    TNode<Context> tmp8;
    TNode<Object> tmp9;
    TNode<BoolT> tmp10;
    TNode<Object> tmp11;
    TNode<Object> tmp12;
    TNode<JSRegExp> tmp13;
    ca_.Bind(&block4, &tmp8, &tmp9, &tmp10, &tmp11, &tmp12, &tmp13);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 73);
    TNode<BoolT> tmp14;
    USE(tmp14);
    tmp14 = CodeStubAssembler(state_).Word32Equal(RegExpBuiltinsAssembler(state_).FastFlagGetterGlobal(TNode<JSRegExp>{tmp13}), CodeStubAssembler(state_).Int32Constant(1));
    TNode<BoolT> tmp15;
    USE(tmp15);
    tmp15 = CodeStubAssembler(state_).Word32Equal(CodeStubAssembler(state_).Word32BinaryNot(TNode<BoolT>{tmp14}), CodeStubAssembler(state_).Int32Constant(1));
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 72);
    ca_.Goto(&block2, tmp8, tmp9, tmp15, tmp11);
  }

  if (block3.is_used()) {
    TNode<Context> tmp16;
    TNode<Object> tmp17;
    TNode<BoolT> tmp18;
    TNode<Object> tmp19;
    ca_.Bind(&block3, &tmp16, &tmp17, &tmp18, &tmp19);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 75);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 76);
    TNode<Object> tmp20;
    USE(tmp20);
    tmp20 = FromConstexpr_JSAny_constexpr_string_0(state_, "flags");
    TNode<Object> tmp21;
    USE(tmp21);
    tmp21 = CodeStubAssembler(state_).GetProperty(TNode<Context>{tmp16}, TNode<Object>{tmp17}, TNode<Object>{tmp20});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 77);
    TNode<Object> tmp22;
    USE(tmp22);
    tmp22 = RequireObjectCoercible_0(state_, TNode<Context>{tmp16}, TNode<Object>{tmp21}, "String.prototype.replaceAll");
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 80);
    TNode<String> tmp23;
    USE(tmp23);
    tmp23 = CodeStubAssembler(state_).ToString_Inline(TNode<Context>{tmp16}, TNode<Object>{tmp21});
    TNode<String> tmp24;
    USE(tmp24);
    tmp24 = CodeStubAssembler(state_).StringConstant("g");
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 79);
    TNode<Smi> tmp25;
    USE(tmp25);
    tmp25 = FromConstexpr_Smi_constexpr_int31_0(state_, 0);
    TNode<Smi> tmp26;
    tmp26 = TORQUE_CAST(CodeStubAssembler(state_).CallBuiltin(Builtins::kStringIndexOf, tmp16, tmp23, tmp24, tmp25));
    USE(tmp26);
    TNode<Smi> tmp27;
    USE(tmp27);
    tmp27 = FromConstexpr_Smi_constexpr_int31_0(state_, -1);
    TNode<BoolT> tmp28;
    USE(tmp28);
    tmp28 = CodeStubAssembler(state_).SmiEqual(TNode<Smi>{tmp26}, TNode<Smi>{tmp27});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 78);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 75);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 72);
    ca_.Goto(&block2, tmp16, tmp17, tmp28, tmp19);
  }

  if (block2.is_used()) {
    TNode<Context> tmp29;
    TNode<Object> tmp30;
    TNode<BoolT> tmp31;
    TNode<Object> tmp32;
    ca_.Bind(&block2, &tmp29, &tmp30, &tmp31, &tmp32);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 71);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 83);
    ca_.Branch(tmp31, &block6, &block7, tmp29, tmp30, tmp31);
  }

  if (block6.is_used()) {
    TNode<Context> tmp33;
    TNode<Object> tmp34;
    TNode<BoolT> tmp35;
    ca_.Bind(&block6, &tmp33, &tmp34, &tmp35);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 84);
    CodeStubAssembler(state_).ThrowTypeError(TNode<Context>{tmp33}, MessageTemplate::kRegExpGlobalInvokedOnNonGlobal, "String.prototype.replaceAll");
  }

  if (block7.is_used()) {
    TNode<Context> tmp36;
    TNode<Object> tmp37;
    TNode<BoolT> tmp38;
    ca_.Bind(&block7, &tmp36, &tmp37, &tmp38);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 69);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 68);
    ca_.Goto(&block1, tmp36, tmp37);
  }

  if (block1.is_used()) {
    TNode<Context> tmp39;
    TNode<Object> tmp40;
    ca_.Bind(&block1, &tmp39, &tmp40);
    ca_.Goto(&block8, tmp39, tmp40);
  }

    TNode<Context> tmp41;
    TNode<Object> tmp42;
    ca_.Bind(&block8, &tmp41, &tmp42);
}

TF_BUILTIN(StringPrototypeReplaceAll, CodeStubAssembler) {
  compiler::CodeAssemblerState* state_ = state();  compiler::CodeAssembler ca_(state());
  TNode<Context> parameter0 = UncheckedCast<Context>(Parameter(Descriptor::kContext));
  USE(parameter0);
  TNode<Object> parameter1 = UncheckedCast<Object>(Parameter(Descriptor::kReceiver));
USE(parameter1);
  TNode<Object> parameter2 = UncheckedCast<Object>(Parameter(Descriptor::kSearchValue));
  USE(parameter2);
  TNode<Object> parameter3 = UncheckedCast<Object>(Parameter(Descriptor::kReplaceValue));
  USE(parameter3);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object, BoolT> block3(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object, BoolT> block4(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object, BoolT, BoolT> block5(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object> block1(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object> block6(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object> block7(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object, Object, Symbol> block11(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object, Object, Symbol, JSReceiver> block10(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object> block9(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object> block8(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object> block2(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object, String, String, Object, BoolT> block12(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object, String, String, Object, BoolT> block13(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object, String, String, Object, BoolT, Smi, Smi, Smi, String, Smi> block16(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object, String, String, Object, BoolT, Smi, Smi, Smi, String, Smi> block14(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object, String, String, Object, BoolT, Smi, Smi, Smi, String, Smi, String> block17(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object, String, String, Object, BoolT, Smi, Smi, Smi, String, Smi, String> block18(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object, String, String, Object, BoolT, Smi, Smi, Smi, String, Smi, String> block19(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object, String, String, Object, BoolT, Smi, Smi, Smi, String, Smi> block15(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object, String, String, Object, BoolT, Smi, Smi, Smi, String, Smi> block20(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Context, Object, Object, Object, String, String, Object, BoolT, Smi, Smi, Smi, String, Smi> block21(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0, parameter0, parameter1, parameter2, parameter3);

  if (block0.is_used()) {
    TNode<Context> tmp0;
    TNode<Object> tmp1;
    TNode<Object> tmp2;
    TNode<Object> tmp3;
    ca_.Bind(&block0, &tmp0, &tmp1, &tmp2, &tmp3);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 94);
    TNode<Object> tmp4;
    USE(tmp4);
    tmp4 = RequireObjectCoercible_0(state_, TNode<Context>{tmp0}, TNode<Object>{tmp1}, "String.prototype.replaceAll");
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 97);
    TNode<Oddball> tmp5;
    USE(tmp5);
    tmp5 = Undefined_0(state_);
    TNode<BoolT> tmp6;
    USE(tmp6);
    tmp6 = CodeStubAssembler(state_).Word32Equal(CodeStubAssembler(state_).TaggedEqual(TNode<Object>{tmp2}, TNode<HeapObject>{tmp5}), CodeStubAssembler(state_).Int32Constant(0));
    ca_.Branch(tmp6, &block3, &block4, tmp0, tmp1, tmp2, tmp3, tmp6);
  }

  if (block3.is_used()) {
    TNode<Context> tmp7;
    TNode<Object> tmp8;
    TNode<Object> tmp9;
    TNode<Object> tmp10;
    TNode<BoolT> tmp11;
    ca_.Bind(&block3, &tmp7, &tmp8, &tmp9, &tmp10, &tmp11);
    TNode<Oddball> tmp12;
    USE(tmp12);
    tmp12 = Null_0(state_);
    TNode<BoolT> tmp13;
    USE(tmp13);
    tmp13 = CodeStubAssembler(state_).Word32Equal(CodeStubAssembler(state_).TaggedEqual(TNode<Object>{tmp9}, TNode<HeapObject>{tmp12}), CodeStubAssembler(state_).Int32Constant(0));
    ca_.Goto(&block5, tmp7, tmp8, tmp9, tmp10, tmp11, tmp13);
  }

  if (block4.is_used()) {
    TNode<Context> tmp14;
    TNode<Object> tmp15;
    TNode<Object> tmp16;
    TNode<Object> tmp17;
    TNode<BoolT> tmp18;
    ca_.Bind(&block4, &tmp14, &tmp15, &tmp16, &tmp17, &tmp18);
    TNode<BoolT> tmp19;
    USE(tmp19);
    tmp19 = FromConstexpr_bool_constexpr_bool_0(state_, false);
    ca_.Goto(&block5, tmp14, tmp15, tmp16, tmp17, tmp18, tmp19);
  }

  if (block5.is_used()) {
    TNode<Context> tmp20;
    TNode<Object> tmp21;
    TNode<Object> tmp22;
    TNode<Object> tmp23;
    TNode<BoolT> tmp24;
    TNode<BoolT> tmp25;
    ca_.Bind(&block5, &tmp20, &tmp21, &tmp22, &tmp23, &tmp24, &tmp25);
    ca_.Branch(tmp25, &block1, &block2, tmp20, tmp21, tmp22, tmp23);
  }

  if (block1.is_used()) {
    TNode<Context> tmp26;
    TNode<Object> tmp27;
    TNode<Object> tmp28;
    TNode<Object> tmp29;
    ca_.Bind(&block1, &tmp26, &tmp27, &tmp28, &tmp29);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 104);
    TNode<BoolT> tmp30;
    USE(tmp30);
    tmp30 = CodeStubAssembler(state_).Word32Equal(RegExpBuiltinsAssembler(state_).IsRegExp0(TNode<Context>{tmp26}, TNode<Object>{tmp28}), CodeStubAssembler(state_).Int32Constant(1));
    ca_.Branch(tmp30, &block6, &block7, tmp26, tmp27, tmp28, tmp29);
  }

  if (block6.is_used()) {
    TNode<Context> tmp31;
    TNode<Object> tmp32;
    TNode<Object> tmp33;
    TNode<Object> tmp34;
    ca_.Bind(&block6, &tmp31, &tmp32, &tmp33, &tmp34);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 105);
    ThrowIfNotGlobal_0(state_, TNode<Context>{tmp31}, TNode<Object>{tmp33});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 104);
    ca_.Goto(&block7, tmp31, tmp32, tmp33, tmp34);
  }

  if (block7.is_used()) {
    TNode<Context> tmp35;
    TNode<Object> tmp36;
    TNode<Object> tmp37;
    TNode<Object> tmp38;
    ca_.Bind(&block7, &tmp35, &tmp36, &tmp37, &tmp38);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 114);
    TNode<Symbol> tmp39;
    USE(tmp39);
    tmp39 = CodeStubAssembler(state_).ReplaceSymbolConstant();
    TNode<JSReceiver> tmp40;
    USE(tmp40);
    compiler::CodeAssemblerLabel label0(&ca_);
    tmp40 = GetMethod_3(state_, TNode<Context>{tmp35}, TNode<Object>{tmp37}, TNode<Symbol>{tmp39}, &label0);
    ca_.Goto(&block10, tmp35, tmp36, tmp37, tmp38, tmp37, tmp39, tmp40);
    if (label0.is_used()) {
      ca_.Bind(&label0);
      ca_.Goto(&block11, tmp35, tmp36, tmp37, tmp38, tmp37, tmp39);
    }
  }

  if (block11.is_used()) {
    TNode<Context> tmp41;
    TNode<Object> tmp42;
    TNode<Object> tmp43;
    TNode<Object> tmp44;
    TNode<Object> tmp45;
    TNode<Symbol> tmp46;
    ca_.Bind(&block11, &tmp41, &tmp42, &tmp43, &tmp44, &tmp45, &tmp46);
    ca_.Goto(&block9, tmp41, tmp42, tmp43, tmp44);
  }

  if (block10.is_used()) {
    TNode<Context> tmp47;
    TNode<Object> tmp48;
    TNode<Object> tmp49;
    TNode<Object> tmp50;
    TNode<Object> tmp51;
    TNode<Symbol> tmp52;
    TNode<JSReceiver> tmp53;
    ca_.Bind(&block10, &tmp47, &tmp48, &tmp49, &tmp50, &tmp51, &tmp52, &tmp53);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 116);
    TNode<Object> tmp54;
    USE(tmp54);
    tmp54 = CodeStubAssembler(state_).Call(TNode<Context>{tmp47}, TNode<JSReceiver>{tmp53}, TNode<Object>{tmp49}, TNode<Object>{tmp48}, TNode<Object>{tmp50});
    CodeStubAssembler(state_).Return(tmp54);
  }

  if (block9.is_used()) {
    TNode<Context> tmp55;
    TNode<Object> tmp56;
    TNode<Object> tmp57;
    TNode<Object> tmp58;
    ca_.Bind(&block9, &tmp55, &tmp56, &tmp57, &tmp58);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 113);
    ca_.Goto(&block8, tmp55, tmp56, tmp57, tmp58);
  }

  if (block8.is_used()) {
    TNode<Context> tmp59;
    TNode<Object> tmp60;
    TNode<Object> tmp61;
    TNode<Object> tmp62;
    ca_.Bind(&block8, &tmp59, &tmp60, &tmp61, &tmp62);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 97);
    ca_.Goto(&block2, tmp59, tmp60, tmp61, tmp62);
  }

  if (block2.is_used()) {
    TNode<Context> tmp63;
    TNode<Object> tmp64;
    TNode<Object> tmp65;
    TNode<Object> tmp66;
    ca_.Bind(&block2, &tmp63, &tmp64, &tmp65, &tmp66);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 122);
    TNode<String> tmp67;
    USE(tmp67);
    tmp67 = CodeStubAssembler(state_).ToString_Inline(TNode<Context>{tmp63}, TNode<Object>{tmp64});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 125);
    TNode<String> tmp68;
    USE(tmp68);
    tmp68 = CodeStubAssembler(state_).ToString_Inline(TNode<Context>{tmp63}, TNode<Object>{tmp65});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 128);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 129);
    TNode<BoolT> tmp69;
    USE(tmp69);
    tmp69 = CodeStubAssembler(state_).TaggedIsCallable(TNode<Object>{tmp66});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 132);
    TNode<BoolT> tmp70;
    USE(tmp70);
    tmp70 =  CodeStubAssembler(state_).Word32Equal(CodeStubAssembler(state_).Word32BinaryNot(TNode<BoolT>{tmp69}), CodeStubAssembler(state_).Int32Constant(1));
    ca_.Branch(tmp70, &block12, &block13, tmp63, tmp64, tmp65, tmp66, tmp67, tmp68, tmp66, tmp69);
  }

  if (block12.is_used()) {
    TNode<Context> tmp71;
    TNode<Object> tmp72;
    TNode<Object> tmp73;
    TNode<Object> tmp74;
    TNode<String> tmp75;
    TNode<String> tmp76;
    TNode<Object> tmp77;
    TNode<BoolT> tmp78;
    ca_.Bind(&block12, &tmp71, &tmp72, &tmp73, &tmp74, &tmp75, &tmp76, &tmp77, &tmp78);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 134);
    TNode<String> tmp79;
    USE(tmp79);
    tmp79 = CodeStubAssembler(state_).ToString_Inline(TNode<Context>{tmp71}, TNode<Object>{tmp74});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 132);
    ca_.Goto(&block13, tmp71, tmp72, tmp73, tmp74, tmp75, tmp76, tmp79, tmp78);
  }

  if (block13.is_used()) {
    TNode<Context> tmp80;
    TNode<Object> tmp81;
    TNode<Object> tmp82;
    TNode<Object> tmp83;
    TNode<String> tmp84;
    TNode<String> tmp85;
    TNode<Object> tmp86;
    TNode<BoolT> tmp87;
    ca_.Bind(&block13, &tmp80, &tmp81, &tmp82, &tmp83, &tmp84, &tmp85, &tmp86, &tmp87);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 138);
    TNode<Smi> tmp88;
    USE(tmp88);
    tmp88 = CodeStubAssembler(state_).LoadStringLengthAsSmi(TNode<String>{tmp85});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 141);
    TNode<Smi> tmp89;
    USE(tmp89);
    tmp89 = FromConstexpr_Smi_constexpr_int31_0(state_, 1);
    TNode<Smi> tmp90;
    USE(tmp90);
    tmp90 = CodeStubAssembler(state_).SmiMax(TNode<Smi>{tmp89}, TNode<Smi>{tmp88});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 155);
    TNode<Smi> tmp91;
    USE(tmp91);
    tmp91 = FromConstexpr_Smi_constexpr_int31_0(state_, 0);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 156);
    TNode<String> tmp92;
    USE(tmp92);
    tmp92 = CodeStubAssembler(state_).EmptyStringConstant();//kEmptyString_0(state_);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 157);
    TNode<Smi> tmp93;
    USE(tmp93);
    tmp93 = FromConstexpr_Smi_constexpr_int31_0(state_, 0);
    TNode<Smi> tmp94;
    USE(tmp94);
    tmp94 = AbstractStringIndexOf_0(state_, TNode<Context>{tmp80}, TNode<String>{tmp84}, TNode<String>{tmp85}, TNode<Smi>{tmp93});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 158);
    ca_.Goto(&block16, tmp80, tmp81, tmp82, tmp83, tmp84, tmp85, tmp86, tmp87, tmp88, tmp90, tmp91, tmp92, tmp94);
  }

  if (block16.is_used()) {
    TNode<Context> tmp95;
    TNode<Object> tmp96;
    TNode<Object> tmp97;
    TNode<Object> tmp98;
    TNode<String> tmp99;
    TNode<String> tmp100;
    TNode<Object> tmp101;
    TNode<BoolT> tmp102;
    TNode<Smi> tmp103;
    TNode<Smi> tmp104;
    TNode<Smi> tmp105;
    TNode<String> tmp106;
    TNode<Smi> tmp107;
    ca_.Bind(&block16, &tmp95, &tmp96, &tmp97, &tmp98, &tmp99, &tmp100, &tmp101, &tmp102, &tmp103, &tmp104, &tmp105, &tmp106, &tmp107);
    TNode<Smi> tmp108;
    USE(tmp108);
    tmp108 = FromConstexpr_Smi_constexpr_int31_0(state_, -1);
    TNode<BoolT> tmp109;
    USE(tmp109);
    tmp109 = CodeStubAssembler(state_).SmiNotEqual(TNode<Smi>{tmp107}, TNode<Smi>{tmp108});
    ca_.Branch(tmp109, &block14, &block15, tmp95, tmp96, tmp97, tmp98, tmp99, tmp100, tmp101, tmp102, tmp103, tmp104, tmp105, tmp106, tmp107);
  }

  if (block14.is_used()) {
    TNode<Context> tmp110;
    TNode<Object> tmp111;
    TNode<Object> tmp112;
    TNode<Object> tmp113;
    TNode<String> tmp114;
    TNode<String> tmp115;
    TNode<Object> tmp116;
    TNode<BoolT> tmp117;
    TNode<Smi> tmp118;
    TNode<Smi> tmp119;
    TNode<Smi> tmp120;
    TNode<String> tmp121;
    TNode<Smi> tmp122;
    ca_.Bind(&block14, &tmp110, &tmp111, &tmp112, &tmp113, &tmp114, &tmp115, &tmp116, &tmp117, &tmp118, &tmp119, &tmp120, &tmp121, &tmp122);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 161);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 162);
    ca_.Branch(tmp117, &block17, &block18, tmp110, tmp111, tmp112, tmp113, tmp114, tmp115, tmp116, tmp117, tmp118, tmp119, tmp120, tmp121, tmp122, ca_.Uninitialized<String>());
  }

  if (block17.is_used()) {
    TNode<Context> tmp123;
    TNode<Object> tmp124;
    TNode<Object> tmp125;
    TNode<Object> tmp126;
    TNode<String> tmp127;
    TNode<String> tmp128;
    TNode<Object> tmp129;
    TNode<BoolT> tmp130;
    TNode<Smi> tmp131;
    TNode<Smi> tmp132;
    TNode<Smi> tmp133;
    TNode<String> tmp134;
    TNode<Smi> tmp135;
    TNode<String> tmp136;
    ca_.Bind(&block17, &tmp123, &tmp124, &tmp125, &tmp126, &tmp127, &tmp128, &tmp129, &tmp130, &tmp131, &tmp132, &tmp133, &tmp134, &tmp135, &tmp136);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 167);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 169);
    TNode<JSReceiver> tmp137;
    USE(tmp137);
    tmp137 = UnsafeCast_Callable_0(state_, TNode<Context>{tmp123}, TNode<Object>{tmp129});
    TNode<Oddball> tmp138;
    USE(tmp138);
    tmp138 = Undefined_0(state_);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 170);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 168);
    TNode<Object> tmp139;
    USE(tmp139);
    tmp139 = CodeStubAssembler(state_).Call(TNode<Context>{tmp123}, TNode<JSReceiver>{tmp137}, TNode<Object>{tmp138}, TNode<Object>{tmp128}, TNode<Object>{tmp135}, TNode<Object>{tmp127});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 166);
    TNode<String> tmp140;
    USE(tmp140);
    tmp140 = CodeStubAssembler(state_).ToString_Inline(TNode<Context>{tmp123}, TNode<Object>{tmp139});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 162);
    ca_.Goto(&block19, tmp123, tmp124, tmp125, tmp126, tmp127, tmp128, tmp129, tmp130, tmp131, tmp132, tmp133, tmp134, tmp135, tmp140);
  }

  if (block18.is_used()) {
    TNode<Context> tmp141;
    TNode<Object> tmp142;
    TNode<Object> tmp143;
    TNode<Object> tmp144;
    TNode<String> tmp145;
    TNode<String> tmp146;
    TNode<Object> tmp147;
    TNode<BoolT> tmp148;
    TNode<Smi> tmp149;
    TNode<Smi> tmp150;
    TNode<Smi> tmp151;
    TNode<String> tmp152;
    TNode<Smi> tmp153;
    TNode<String> tmp154;
    ca_.Bind(&block18, &tmp141, &tmp142, &tmp143, &tmp144, &tmp145, &tmp146, &tmp147, &tmp148, &tmp149, &tmp150, &tmp151, &tmp152, &tmp153, &tmp154);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 173);
    TNode<String> tmp155;
    USE(tmp155);
    tmp155 = UnsafeCast_String_0(state_, TNode<Context>{tmp141}, TNode<Object>{tmp147});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 180);
    TNode<Smi> tmp156;
    USE(tmp156);
    tmp156 = CodeStubAssembler(state_).SmiAdd(TNode<Smi>{tmp153}, TNode<Smi>{tmp149});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 182);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 181);
    TNode<String> tmp157;
    USE(tmp157);
    tmp157 = CodeStubAssembler(state_).ToString(TNode<Context>{tmp141},StringBuiltinsAssembler(state_).GetSubstitution(TNode<Context>{tmp141}, TNode<String>{tmp145}, TNode<Smi>{tmp153}, TNode<Smi>{tmp156}, TNode<String>{tmp155}));
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 171);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 162);
    ca_.Goto(&block19, tmp141, tmp142, tmp143, tmp144, tmp145, tmp146, tmp147, tmp148, tmp149, tmp150, tmp151, tmp152, tmp153, tmp157);
  }

  if (block19.is_used()) {
    TNode<Context> tmp158;
    TNode<Object> tmp159;
    TNode<Object> tmp160;
    TNode<Object> tmp161;
    TNode<String> tmp162;
    TNode<String> tmp163;
    TNode<Object> tmp164;
    TNode<BoolT> tmp165;
    TNode<Smi> tmp166;
    TNode<Smi> tmp167;
    TNode<Smi> tmp168;
    TNode<String> tmp169;
    TNode<Smi> tmp170;
    TNode<String> tmp171;
    ca_.Bind(&block19, &tmp158, &tmp159, &tmp160, &tmp161, &tmp162, &tmp163, &tmp164, &tmp165, &tmp166, &tmp167, &tmp168, &tmp169, &tmp170, &tmp171);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 189);
    TNode<IntPtrT> tmp172;
    USE(tmp172);
    tmp172 = CodeStubAssembler(state_).SmiUntag(TNode<Smi>{tmp168});
    TNode<UintPtrT> tmp173;
    USE(tmp173);
    tmp173 = CodeStubAssembler(state_).Unsigned(TNode<IntPtrT>{tmp172});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 190);
    TNode<IntPtrT> tmp174;
    USE(tmp174);
    tmp174 = CodeStubAssembler(state_).SmiUntag(TNode<Smi>{tmp170});
    TNode<UintPtrT> tmp175;
    USE(tmp175);
    tmp175 = CodeStubAssembler(state_).Unsigned(TNode<IntPtrT>{tmp174});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 188);
    TNode<String> tmp176;
    USE(tmp176);
    tmp176 = StringBuiltinsAssembler(state_).SubString(TNode<String>{tmp162}, TNode<UintPtrT>{tmp173}, TNode<UintPtrT>{tmp175});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 196);
    TNode<String> tmp177;
    USE(tmp177);
    tmp177 = StringBuiltinsAssembler(state_).StringAdd( TNode<Context>{tmp158}, TNode<String>{tmp169}, TNode<String>{tmp176});
    TNode<String> tmp178;
    USE(tmp178);
    tmp178 = StringBuiltinsAssembler(state_).StringAdd(TNode<Context>{tmp158}, TNode<String>{tmp177}, TNode<String>{tmp171});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 199);
    TNode<Smi> tmp179;
    USE(tmp179);
    tmp179 = CodeStubAssembler(state_).SmiAdd(TNode<Smi>{tmp170}, TNode<Smi>{tmp166});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 202);
    TNode<Smi> tmp180;
    USE(tmp180);
    tmp180 = CodeStubAssembler(state_).SmiAdd(TNode<Smi>{tmp170}, TNode<Smi>{tmp167});
    TNode<Smi> tmp181;
    USE(tmp181);
    tmp181 = AbstractStringIndexOf_0(state_, TNode<Context>{tmp158}, TNode<String>{tmp162}, TNode<String>{tmp163}, TNode<Smi>{tmp180});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 201);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 158);
    ca_.Goto(&block16, tmp158, tmp159, tmp160, tmp161, tmp162, tmp163, tmp164, tmp165, tmp166, tmp167, tmp179, tmp178, tmp181);
  }

  if (block15.is_used()) {
    TNode<Context> tmp182;
    TNode<Object> tmp183;
    TNode<Object> tmp184;
    TNode<Object> tmp185;
    TNode<String> tmp186;
    TNode<String> tmp187;
    TNode<Object> tmp188;
    TNode<BoolT> tmp189;
    TNode<Smi> tmp190;
    TNode<Smi> tmp191;
    TNode<Smi> tmp192;
    TNode<String> tmp193;
    TNode<Smi> tmp194;
    ca_.Bind(&block15, &tmp182, &tmp183, &tmp184, &tmp185, &tmp186, &tmp187, &tmp188, &tmp189, &tmp190, &tmp191, &tmp192, &tmp193, &tmp194);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 206);
    TNode<Smi> tmp195;
    USE(tmp195);
    tmp195 = CodeStubAssembler(state_).LoadStringLengthAsSmi(TNode<String>{tmp186});
    TNode<BoolT> tmp196;
    USE(tmp196);
    tmp196 = CodeStubAssembler(state_).SmiLessThan(TNode<Smi>{tmp192}, TNode<Smi>{tmp195});
    ca_.Branch(tmp196, &block20, &block21, tmp182, tmp183, tmp184, tmp185, tmp186, tmp187, tmp188, tmp189, tmp190, tmp191, tmp192, tmp193, tmp194);
  }

  if (block20.is_used()) {
    TNode<Context> tmp197;
    TNode<Object> tmp198;
    TNode<Object> tmp199;
    TNode<Object> tmp200;
    TNode<String> tmp201;
    TNode<String> tmp202;
    TNode<Object> tmp203;
    TNode<BoolT> tmp204;
    TNode<Smi> tmp205;
    TNode<Smi> tmp206;
    TNode<Smi> tmp207;
    TNode<String> tmp208;
    TNode<Smi> tmp209;
    ca_.Bind(&block20, &tmp197, &tmp198, &tmp199, &tmp200, &tmp201, &tmp202, &tmp203, &tmp204, &tmp205, &tmp206, &tmp207, &tmp208, &tmp209);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 210);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 212);
    TNode<IntPtrT> tmp210;
    USE(tmp210);
    tmp210 = CodeStubAssembler(state_).SmiUntag(TNode<Smi>{tmp207});
    TNode<UintPtrT> tmp211;
    USE(tmp211);
    tmp211 = CodeStubAssembler(state_).Unsigned(TNode<IntPtrT>{tmp210});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 213);
    TNode<IntPtrT> tmp212;
    USE(tmp212);
    tmp212 = CodeStubAssembler(state_).LoadStringLengthAsWord(TNode<String>{tmp201});
    TNode<UintPtrT> tmp213;
    USE(tmp213);
    tmp213 = CodeStubAssembler(state_).Unsigned(TNode<IntPtrT>{tmp212});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 211);
    TNode<String> tmp214;
    USE(tmp214);
    tmp214 = StringBuiltinsAssembler(state_).SubString(TNode<String>{tmp201},TNode<UintPtrT>{tmp211}, TNode<UintPtrT>{tmp213});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 210);
    TNode<String> tmp215;
    USE(tmp215);
    tmp215 = StringBuiltinsAssembler(state_).StringAdd( TNode<Context>{tmp197}, TNode<String>{tmp208}, TNode<String>{tmp214});
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 206);
    ca_.Goto(&block21, tmp197, tmp198, tmp199, tmp200, tmp201, tmp202, tmp203, tmp204, tmp205, tmp206, tmp207, tmp215, tmp209);
  }

  if (block21.is_used()) {
    TNode<Context> tmp216;
    TNode<Object> tmp217;
    TNode<Object> tmp218;
    TNode<Object> tmp219;
    TNode<String> tmp220;
    TNode<String> tmp221;
    TNode<Object> tmp222;
    TNode<BoolT> tmp223;
    TNode<Smi> tmp224;
    TNode<Smi> tmp225;
    TNode<Smi> tmp226;
    TNode<String> tmp227;
    TNode<Smi> tmp228;
    ca_.Bind(&block21, &tmp216, &tmp217, &tmp218, &tmp219, &tmp220, &tmp221, &tmp222, &tmp223, &tmp224, &tmp225, &tmp226, &tmp227, &tmp228);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 217);
    CodeStubAssembler(state_).Return(tmp227);
  }
}

TNode<Number> Convert_Number_Smi_0(compiler::CodeAssemblerState* state_, TNode<Smi> p_i) {
  compiler::CodeAssembler ca_(state_);
  compiler::CodeAssemblerParameterizedLabel<Smi> block0(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Smi, Number> block1(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
  compiler::CodeAssemblerParameterizedLabel<Smi, Number> block2(&ca_, compiler::CodeAssemblerLabel::kNonDeferred);
    ca_.Goto(&block0, p_i);

  if (block0.is_used()) {
    TNode<Smi> tmp0;
    ca_.Bind(&block0, &tmp0);
    //ca_.SetSourcePosition("../../v8/src/builtins/base.tq", 3037);
    ca_.Goto(&block1, tmp0, tmp0);
  }

  if (block1.is_used()) {
    TNode<Smi> tmp1;
    TNode<Number> tmp2;
    ca_.Bind(&block1, &tmp1, &tmp2);
    //ca_.SetSourcePosition("../../v8/src/builtins/string-replaceall.tq", 59);
    ca_.Goto(&block2, tmp1, tmp2);
  }

    TNode<Smi> tmp3;
    TNode<Number> tmp4;
    ca_.Bind(&block2, &tmp3, &tmp4);
  return TNode<Number>{tmp4};
}

}  // namespace internal
}  // namespace v8


