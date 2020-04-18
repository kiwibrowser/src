var testCases = [
    {
        name: 'RemoveSimple',
        precondition: [
            {fullPath:'/a', isDirectory:true},
            {fullPath:'/b'}
        ],
        tests: [
            function(helper) { helper.remove('/a'); },
            function(helper) { helper.remove('/b'); },
            function(helper) { helper.remove('/', 'InvalidModificationError'); }
        ],
        postcondition: [
            {fullPath:'/a', nonexistent:true},
            {fullPath:'/b', nonexistent:true}
        ],
    },
    {
        name: 'RemoveNonRecursiveWithChildren',
        precondition: [
            {fullPath:'/a', isDirectory:true},
            {fullPath:'/a/b', isDirectory:true},
            {fullPath:'/a/c',}
        ],
        tests: [
            function(helper) { helper.remove('/a', 'InvalidModificationError'); }
        ],
        postcondition: [
            {fullPath:'/a', isDirectory:true},
            {fullPath:'/a/b', isDirectory:true},
            {fullPath:'/a/c',}
        ],
    },
    {
        name: 'RemoveRecursiveWithoutChildren',
        precondition: [
            {fullPath:'/a', isDirectory:true},
        ],
        tests: [
            function(helper) { helper.removeRecursively('/a'); }
        ],
        postcondition: [
            {fullPath:'/a', nonexistent:true},
        ],
    },
    {
        name: 'RemoveRecursiveWithChildren',
        precondition: [
            {fullPath:'/a', isDirectory:true},
            {fullPath:'/a/b', isDirectory:true},
            {fullPath:'/a/c',}
        ],
        tests: [
            function(helper) { helper.removeRecursively('/a'); },
            function(helper) { helper.removeRecursively('/', 'InvalidModificationError'); }
        ],
        postcondition: [
            {fullPath:'/a', nonexistent:true},
        ],
    },
];

