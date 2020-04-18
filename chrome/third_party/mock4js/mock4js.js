/**
 * Mock4JS 0.2
 * http://mock4js.sourceforge.net/
 */

Mock4JS = {
	_mocksToVerify: [],
	_convertToConstraint: function(constraintOrValue) {
		if(constraintOrValue.argumentMatches) {
			return constraintOrValue; // it's already an ArgumentMatcher
		} else {
			return new MatchExactly(constraintOrValue);	// default to eq(...)
		}
	},
	addMockSupport: function(object) {
		// mock creation
		object.mock = function(mockedType) {
			if(!mockedType) {
				throw new Mock4JSException("Cannot create mock: type to mock cannot be found or is null");
			}
			var newMock = new Mock(mockedType);
			Mock4JS._mocksToVerify.push(newMock);
			return newMock;
		}

		// syntactic sugar for expects()
		object.once = function() {
			return new CallCounter(1);
		}
		object.never = function() {
			return new CallCounter(0);
		}
		object.exactly = function(expectedCallCount) {
			return new CallCounter(expectedCallCount);
		}
		object.atLeastOnce = function() {
			return new InvokeAtLeastOnce();
		}
		
		// syntactic sugar for argument expectations
		object.ANYTHING = new MatchAnything();
		object.NOT_NULL = new MatchAnythingBut(new MatchExactly(null));
		object.NOT_UNDEFINED = new MatchAnythingBut(new MatchExactly(undefined));
		object.eq = function(expectedValue) {
			return new MatchExactly(expectedValue);
		}
		object.not = function(valueNotExpected) {
			var argConstraint = Mock4JS._convertToConstraint(valueNotExpected);
			return new MatchAnythingBut(argConstraint);
		}
		object.and = function() {
			var constraints = [];
			for(var i=0; i<arguments.length; i++) {
				constraints[i] = Mock4JS._convertToConstraint(arguments[i]);
			}
			return new MatchAllOf(constraints);
		}
		object.or = function() {
			var constraints = [];
			for(var i=0; i<arguments.length; i++) {
				constraints[i] = Mock4JS._convertToConstraint(arguments[i]);
			}
			return new MatchAnyOf(constraints);
		}
		object.stringContains = function(substring) {
			return new MatchStringContaining(substring);
		}
		
		// syntactic sugar for will()
		object.returnValue = function(value) {
			return new ReturnValueAction(value);
		}
		object.throwException = function(exception) {
			return new ThrowExceptionAction(exception);
		}
	},
	clearMocksToVerify: function() {
		Mock4JS._mocksToVerify = [];
	},
	verifyAllMocks: function() {
		for(var i=0; i<Mock4JS._mocksToVerify.length; i++) {
			Mock4JS._mocksToVerify[i].verify();
		}
	}
}

Mock4JSUtil = {
	hasFunction: function(obj, methodName) {
		return typeof obj == 'object' && typeof obj[methodName] == 'function';
	},
	join: function(list) {
		var result = "";
		for(var i=0; i<list.length; i++) {
			var item = list[i];
			if(Mock4JSUtil.hasFunction(item, "describe")) {
				result += item.describe();
			}
			else if(typeof list[i] == 'string') {
				result += "\""+list[i]+"\"";
			} else {
				result += list[i];
			}
			
			if(i<list.length-1) result += ", ";
		}
		return result;
	}	
}

Mock4JSException = function(message) {
	this.message = message;
}

Mock4JSException.prototype = {
	toString: function() {
		return this.message;
	}
}

/**
 * Assert function that makes use of the constraint methods
 */ 
assertThat = function(expected, argumentMatcher) {
	if(!argumentMatcher.argumentMatches(expected)) {
		throw new Mock4JSException("Expected '"+expected+"' to be "+argumentMatcher.describe());
	}
}

/**
 * CallCounter
 */
function CallCounter(expectedCount) {
	this._expectedCallCount = expectedCount;
	this._actualCallCount = 0;
}

CallCounter.prototype = {
	addActualCall: function() {
		this._actualCallCount++;
		if(this._actualCallCount > this._expectedCallCount) {
			throw new Mock4JSException("unexpected invocation");
		}
	},
	
	verify: function() {
		if(this._actualCallCount < this._expectedCallCount) {
			throw new Mock4JSException("expected method was not invoked the expected number of times");
		}
	},
	
	describe: function() {
		if(this._expectedCallCount == 0) {
			return "not expected";
		} else if(this._expectedCallCount == 1) {
			var msg = "expected once";
			if(this._actualCallCount >= 1) {
				msg += " and has been invoked";
			}
			return msg;
		} else {
			var msg = "expected "+this._expectedCallCount+" times";
			if(this._actualCallCount > 0) {
				msg += ", invoked "+this._actualCallCount + " times";
			}
			return msg;
		}
	}
}

function InvokeAtLeastOnce() {
	this._hasBeenInvoked = false;
}

InvokeAtLeastOnce.prototype = {
	addActualCall: function() {
		this._hasBeenInvoked = true;
	},
	
	verify: function() {
		if(this._hasBeenInvoked === false) {
			throw new Mock4JSException(describe());
		}
	},
	
	describe: function() {
		var desc = "expected at least once";
		if(this._hasBeenInvoked) desc+=" and has been invoked";
		return desc;
	}
}

/**
 * ArgumentMatchers
 */

function MatchExactly(expectedValue) {
	this._expectedValue = expectedValue;
}

MatchExactly.prototype = {
	argumentMatches: function(actualArgument) {
		if(this._expectedValue instanceof Array) {
			if(!(actualArgument instanceof Array)) return false;
			if(this._expectedValue.length != actualArgument.length) return false;
			for(var i=0; i<this._expectedValue.length; i++) {
				if(this._expectedValue[i] != actualArgument[i]) return false;
			}
			return true;
		} else {
			return this._expectedValue == actualArgument;
		}
	},
	describe: function() {
		if(typeof this._expectedValue == "string") {
			return "eq(\""+this._expectedValue+"\")";
		} else {
			return "eq("+this._expectedValue+")";
		}
	}
}

function MatchAnything() {
}

MatchAnything.prototype = {
	argumentMatches: function(actualArgument) {
		return true;
	},
	describe: function() {
		return "ANYTHING";
	}
}

function MatchAnythingBut(matcherToNotMatch) {
	this._matcherToNotMatch = matcherToNotMatch;
}

MatchAnythingBut.prototype = {
	argumentMatches: function(actualArgument) {
		return !this._matcherToNotMatch.argumentMatches(actualArgument);
	},
	describe: function() {
		return "not("+this._matcherToNotMatch.describe()+")";
	}
}

function MatchAllOf(constraints) {
	this._constraints = constraints;
}


MatchAllOf.prototype = {
	argumentMatches: function(actualArgument) {
		for(var i=0; i<this._constraints.length; i++) {
			var constraint = this._constraints[i];
			if(!constraint.argumentMatches(actualArgument)) return false;
		}
		return true;
	},
	describe: function() {
		return "and("+Mock4JSUtil.join(this._constraints)+")";
	}
}

function MatchAnyOf(constraints) {
	this._constraints = constraints;
}

MatchAnyOf.prototype = {
	argumentMatches: function(actualArgument) {
		for(var i=0; i<this._constraints.length; i++) {
			var constraint = this._constraints[i];
			if(constraint.argumentMatches(actualArgument)) return true;
		}
		return false;
	},
	describe: function() {
		return "or("+Mock4JSUtil.join(this._constraints)+")";
	}
}


function MatchStringContaining(stringToLookFor) {
	this._stringToLookFor = stringToLookFor;
}

MatchStringContaining.prototype = {
	argumentMatches: function(actualArgument) {
		if(typeof actualArgument != 'string') throw new Mock4JSException("stringContains() must be given a string, actually got a "+(typeof actualArgument));
		return (actualArgument.indexOf(this._stringToLookFor) != -1);
	},
	describe: function() {
		return "a string containing \""+this._stringToLookFor+"\"";
	}
}


/**
 * StubInvocation
 */
function StubInvocation(expectedMethodName, expectedArgs, actionSequence) {
	this._expectedMethodName = expectedMethodName;
	this._expectedArgs = expectedArgs;
	this._actionSequence = actionSequence;
}

StubInvocation.prototype = {
	matches: function(invokedMethodName, invokedMethodArgs) {
		if (invokedMethodName != this._expectedMethodName) {
			return false;
		}
		
		if (invokedMethodArgs.length != this._expectedArgs.length) {
			return false;
		}
		
		for(var i=0; i<invokedMethodArgs.length; i++) {
			var expectedArg = this._expectedArgs[i];
			var invokedArg = invokedMethodArgs[i];
			if(!expectedArg.argumentMatches(invokedArg)) {
				return false;
			}
		}
		
		return true;
	},
	
	invoked: function() {
		try {
			return this._actionSequence.invokeNextAction();
		} catch(e) {
			if(e instanceof Mock4JSException) {
				throw new Mock4JSException(this.describeInvocationNameAndArgs()+" - "+e.message);
			} else {
				throw e;
			}
		}
	},
	
	will: function() {
		this._actionSequence.addAll.apply(this._actionSequence, arguments);
	},
	
	describeInvocationNameAndArgs: function() {
		return this._expectedMethodName+"("+Mock4JSUtil.join(this._expectedArgs)+")";
	},
	
	describe: function() {
		return "stub: "+this.describeInvocationNameAndArgs();
	},
	
	verify: function() {
	}
}

/**
 * ExpectedInvocation
 */
function ExpectedInvocation(expectedMethodName, expectedArgs, expectedCallCounter) {
	this._stubInvocation = new StubInvocation(expectedMethodName, expectedArgs, new ActionSequence());
	this._expectedCallCounter = expectedCallCounter;
}

ExpectedInvocation.prototype = {
	matches: function(invokedMethodName, invokedMethodArgs) {
		try {
			return this._stubInvocation.matches(invokedMethodName, invokedMethodArgs);
		} catch(e) {
			throw new Mock4JSException("method "+this._stubInvocation.describeInvocationNameAndArgs()+": "+e.message);
		}
	},
	
	invoked: function() {
		try {
			this._expectedCallCounter.addActualCall();
		} catch(e) {
			throw new Mock4JSException(e.message+": "+this._stubInvocation.describeInvocationNameAndArgs());
		}
		return this._stubInvocation.invoked();
	},
	
	will: function() {
		this._stubInvocation.will.apply(this._stubInvocation, arguments);
	},
	
	describe: function() {
		return this._expectedCallCounter.describe()+": "+this._stubInvocation.describeInvocationNameAndArgs();
	},
	
	verify: function() {
		try {
			this._expectedCallCounter.verify();
		} catch(e) {
			throw new Mock4JSException(e.message+": "+this._stubInvocation.describeInvocationNameAndArgs());
		}
	}
}

/**
 * MethodActions
 */
function ReturnValueAction(valueToReturn) {
	this._valueToReturn = valueToReturn;
}

ReturnValueAction.prototype = {
	invoke: function() {
		return this._valueToReturn;
	},
	describe: function() {
		return "returns "+this._valueToReturn;
	}
}

function ThrowExceptionAction(exceptionToThrow) {
	this._exceptionToThrow = exceptionToThrow;
}

ThrowExceptionAction.prototype = {
	invoke: function() {
		throw this._exceptionToThrow;
	},
	describe: function() {
		return "throws "+this._exceptionToThrow;
	}
}

function ActionSequence() {
	this._ACTIONS_NOT_SETUP = "_ACTIONS_NOT_SETUP";
	this._actionSequence = this._ACTIONS_NOT_SETUP;
	this._indexOfNextAction = 0;
}

ActionSequence.prototype = {
	invokeNextAction: function() {
		if(this._actionSequence === this._ACTIONS_NOT_SETUP) {
			return;
		} else {
			if(this._indexOfNextAction >= this._actionSequence.length) {
				throw new Mock4JSException("no more values to return");
			} else {
				var action = this._actionSequence[this._indexOfNextAction];
				this._indexOfNextAction++;
				return action.invoke();
			}
		}
	},
	
	addAll: function() {
		this._actionSequence = [];
		for(var i=0; i<arguments.length; i++) {
			if(typeof arguments[i] != 'object' && arguments[i].invoke === undefined) {
				throw new Error("cannot add a method action that does not have an invoke() method");
			}
			this._actionSequence.push(arguments[i]);
		}
	}
}

function StubActionSequence() {
	this._ACTIONS_NOT_SETUP = "_ACTIONS_NOT_SETUP";
	this._actionSequence = this._ACTIONS_NOT_SETUP;
	this._indexOfNextAction = 0;
} 

StubActionSequence.prototype = {
	invokeNextAction: function() {
		if(this._actionSequence === this._ACTIONS_NOT_SETUP) {
			return;
		} else if(this._actionSequence.length == 1) {
			// if there is only one method action, keep doing that on every invocation
			return this._actionSequence[0].invoke();
		} else {
			if(this._indexOfNextAction >= this._actionSequence.length) {
				throw new Mock4JSException("no more values to return");
			} else {
				var action = this._actionSequence[this._indexOfNextAction];
				this._indexOfNextAction++;
				return action.invoke();
			}
		}
	},
	
	addAll: function() {
		this._actionSequence = [];
		for(var i=0; i<arguments.length; i++) {
			if(typeof arguments[i] != 'object' && arguments[i].invoke === undefined) {
				throw new Error("cannot add a method action that does not have an invoke() method");
			}
			this._actionSequence.push(arguments[i]);
		}
	}
}

 
/**
 * Mock
 */
function Mock(mockedType) {
	if(mockedType === undefined || mockedType.prototype === undefined) {
		throw new Mock4JSException("Unable to create Mock: must create Mock using a class not prototype, eg. 'new Mock(TypeToMock)' or using the convenience method 'mock(TypeToMock)'");
	}
	this._mockedType = mockedType.prototype;
	this._expectedCallCount;
	this._isRecordingExpectations = false;
	this._expectedInvocations = [];

	// setup proxy
	var IntermediateClass = new Function();
	IntermediateClass.prototype = mockedType.prototype;
	var ChildClass = new Function();
	ChildClass.prototype = new IntermediateClass();
	this._proxy = new ChildClass();
	this._proxy.mock = this;
	
	for(property in mockedType.prototype) {
		if(this._isPublicMethod(mockedType.prototype, property)) {
			var publicMethodName = property;
			this._proxy[publicMethodName] = this._createMockedMethod(publicMethodName);
			this[publicMethodName] = this._createExpectationRecordingMethod(publicMethodName);
		}
	}
}

Mock.prototype = {
	
	proxy: function() {
		return this._proxy;
	},
	
	expects: function(expectedCallCount) {
		this._expectedCallCount = expectedCallCount;
		this._isRecordingExpectations = true;
		this._isRecordingStubs = false;
		return this;
	},
	
	stubs: function() {
		this._isRecordingExpectations = false;
		this._isRecordingStubs = true;
		return this;
	},
	
	verify: function() {
		for(var i=0; i<this._expectedInvocations.length; i++) {
			var expectedInvocation = this._expectedInvocations[i];
			try {
				expectedInvocation.verify();
			} catch(e) {
				var failMsg = e.message+this._describeMockSetup();
				throw new Mock4JSException(failMsg);
			}
		}
	},
	
	_isPublicMethod: function(mockedType, property) {
		try {
			var isMethod = typeof(mockedType[property]) == 'function';
			var isPublic = property.charAt(0) != "_"; 
			return isMethod && isPublic;
		} catch(e) {
			return false;
		}
	},

	_createExpectationRecordingMethod: function(methodName) {
		return function() {
			// ensure all arguments are instances of ArgumentMatcher
			var expectedArgs = [];
			for(var i=0; i<arguments.length; i++) {
				if(arguments[i] !== null && arguments[i] !== undefined && arguments[i].argumentMatches) {
					expectedArgs[i] = arguments[i];
				} else {
					expectedArgs[i] = new MatchExactly(arguments[i]);
				}
			}
			
			// create stub or expected invocation
			var expectedInvocation;
			if(this._isRecordingExpectations) {
				expectedInvocation = new ExpectedInvocation(methodName, expectedArgs, this._expectedCallCount);
			} else {
				expectedInvocation = new StubInvocation(methodName, expectedArgs, new StubActionSequence());
			}
			
			this._expectedInvocations.push(expectedInvocation);
			
			this._isRecordingExpectations = false;
			this._isRecordingStubs = false;
			return expectedInvocation;
		}
	},
	
	_createMockedMethod: function(methodName) {
		return function() {
			// go through expectation list backwards to ensure later expectations override earlier ones
			for(var i=this.mock._expectedInvocations.length-1; i>=0; i--) {
				var expectedInvocation = this.mock._expectedInvocations[i];
				if(expectedInvocation.matches(methodName, arguments)) {
					try {
						return expectedInvocation.invoked();
					} catch(e) {
						if(e instanceof Mock4JSException) {
							throw new Mock4JSException(e.message+this.mock._describeMockSetup());
						} else {
							// the user setup the mock to throw a specific error, so don't modify the message
							throw e;
						}
					}
				}
			}
			var failMsg = "unexpected invocation: "+methodName+"("+Mock4JSUtil.join(arguments)+")"+this.mock._describeMockSetup();
			throw new Mock4JSException(failMsg);
		};
	},
	
	_describeMockSetup: function() {
		var msg = "\nAllowed:";
		for(var i=0; i<this._expectedInvocations.length; i++) {
			var expectedInvocation = this._expectedInvocations[i];
			msg += "\n" + expectedInvocation.describe();
		}
		return msg;
	}
}
