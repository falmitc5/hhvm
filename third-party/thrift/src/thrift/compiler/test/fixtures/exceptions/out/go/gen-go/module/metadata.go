// Autogenerated by Thrift for thrift/compiler/test/fixtures/exceptions/src/module.thrift
//
// DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
//  @generated

package module

import (
    "maps"
    "sync"

    thrift "github.com/facebook/fbthrift/thrift/lib/go/thrift/types"
    metadata "github.com/facebook/fbthrift/thrift/lib/thrift/metadata"
)

// (needed to ensure safety because of naive import list construction)
var _ = thrift.ZERO
var _ = maps.Copy[map[int]int, map[int]int]
var _ = metadata.GoUnusedProtection__

// Premade Thrift types
var (
    premadeThriftType_string *metadata.ThriftType = nil
    premadeThriftType_module_Fiery *metadata.ThriftType = nil
    premadeThriftType_module_Serious *metadata.ThriftType = nil
    premadeThriftType_module_ComplexFieldNames *metadata.ThriftType = nil
    premadeThriftType_module_CustomFieldNames *metadata.ThriftType = nil
    premadeThriftType_i32 *metadata.ThriftType = nil
    premadeThriftType_module_ExceptionWithPrimitiveField *metadata.ThriftType = nil
    premadeThriftType_module_ExceptionWithStructuredAnnotation *metadata.ThriftType = nil
    premadeThriftType_module_Banal *metadata.ThriftType = nil
    premadeThriftType_void *metadata.ThriftType = nil
)

// Premade Thrift type initializer
var premadeThriftTypesInitOnce = sync.OnceFunc(func() {
    premadeThriftType_string = metadata.NewThriftType().SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_STRING_TYPE.Ptr(),
    )
    premadeThriftType_module_Fiery = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.Fiery"),
    )
    premadeThriftType_module_Serious = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.Serious"),
    )
    premadeThriftType_module_ComplexFieldNames = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.ComplexFieldNames"),
    )
    premadeThriftType_module_CustomFieldNames = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.CustomFieldNames"),
    )
    premadeThriftType_i32 = metadata.NewThriftType().SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
    )
    premadeThriftType_module_ExceptionWithPrimitiveField = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.ExceptionWithPrimitiveField"),
    )
    premadeThriftType_module_ExceptionWithStructuredAnnotation = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.ExceptionWithStructuredAnnotation"),
    )
    premadeThriftType_module_Banal = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.Banal"),
    )
    premadeThriftType_void = metadata.NewThriftType().SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_VOID_TYPE.Ptr(),
    )
})

// Helper type to allow us to store Thrift types in a slice at compile time,
// and put them in a map at runtime. See comment at the top of template
// about a compilation limitation that affects map literals.
type thriftTypeWithFullName struct {
    fullName   string
    thriftType *metadata.ThriftType
}

var premadeThriftTypesMapOnce = sync.OnceValue(
    func() map[string]*metadata.ThriftType {
        // Relies on premade Thrift types initialization
        premadeThriftTypesInitOnce()

        thriftTypesWithFullName := make([]thriftTypeWithFullName, 0)
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "string", premadeThriftType_string })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "module.Fiery", premadeThriftType_module_Fiery })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "module.Serious", premadeThriftType_module_Serious })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "module.ComplexFieldNames", premadeThriftType_module_ComplexFieldNames })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "module.CustomFieldNames", premadeThriftType_module_CustomFieldNames })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "i32", premadeThriftType_i32 })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "module.ExceptionWithPrimitiveField", premadeThriftType_module_ExceptionWithPrimitiveField })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "module.ExceptionWithStructuredAnnotation", premadeThriftType_module_ExceptionWithStructuredAnnotation })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "module.Banal", premadeThriftType_module_Banal })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "void", premadeThriftType_void })

        fbthriftThriftTypesMap := make(map[string]*metadata.ThriftType, len(thriftTypesWithFullName))
        for _, value := range thriftTypesWithFullName {
            fbthriftThriftTypesMap[value.fullName] = value.thriftType
        }
        return fbthriftThriftTypesMap
    },
)

var structMetadatasOnce = sync.OnceValue(
    func() []*metadata.ThriftStruct {
        // Relies on premade Thrift types initialization
        premadeThriftTypesInitOnce()

        fbthriftResults := make([]*metadata.ThriftStruct, 0)
        return fbthriftResults
    },
)

var exceptionMetadatasOnce = sync.OnceValue(
    func() []*metadata.ThriftException {
        // Relies on premade Thrift types initialization
        premadeThriftTypesInitOnce()

        fbthriftResults := make([]*metadata.ThriftException, 0)
        fbthriftResults = append(fbthriftResults, metadata.NewThriftException().
    SetName("module.Fiery").
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("message").
    SetIsOptional(false).
    SetType(premadeThriftType_string),
        },
    ))
        fbthriftResults = append(fbthriftResults, metadata.NewThriftException().
    SetName("module.Serious").
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("sonnet").
    SetIsOptional(true).
    SetType(premadeThriftType_string),
        },
    ))
        fbthriftResults = append(fbthriftResults, metadata.NewThriftException().
    SetName("module.ComplexFieldNames").
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("error_message").
    SetIsOptional(false).
    SetType(premadeThriftType_string),
            metadata.NewThriftField().
    SetId(2).
    SetName("internal_error_message").
    SetIsOptional(false).
    SetType(premadeThriftType_string),
        },
    ))
        fbthriftResults = append(fbthriftResults, metadata.NewThriftException().
    SetName("module.CustomFieldNames").
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("error_message").
    SetIsOptional(false).
    SetType(premadeThriftType_string),
            metadata.NewThriftField().
    SetId(2).
    SetName("internal_error_message").
    SetIsOptional(false).
    SetType(premadeThriftType_string),
        },
    ))
        fbthriftResults = append(fbthriftResults, metadata.NewThriftException().
    SetName("module.ExceptionWithPrimitiveField").
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("message").
    SetIsOptional(false).
    SetType(premadeThriftType_string),
            metadata.NewThriftField().
    SetId(2).
    SetName("error_code").
    SetIsOptional(false).
    SetType(premadeThriftType_i32),
        },
    ))
        fbthriftResults = append(fbthriftResults, metadata.NewThriftException().
    SetName("module.ExceptionWithStructuredAnnotation").
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("message_field").
    SetIsOptional(false).
    SetType(premadeThriftType_string),
            metadata.NewThriftField().
    SetId(2).
    SetName("error_code").
    SetIsOptional(false).
    SetType(premadeThriftType_i32),
        },
    ))
        fbthriftResults = append(fbthriftResults, metadata.NewThriftException().
    SetName("module.Banal"))
        return fbthriftResults
    },
)

var enumMetadatasOnce = sync.OnceValue(
    func() []*metadata.ThriftEnum {
        // Relies on premade Thrift types initialization
        premadeThriftTypesInitOnce()

        fbthriftResults := make([]*metadata.ThriftEnum, 0)
        return fbthriftResults
    },
)

var serviceMetadatasOnce = sync.OnceValue(
    func() []*metadata.ThriftService {
        // Relies on premade Thrift types initialization
        premadeThriftTypesInitOnce()

        fbthriftResults := make([]*metadata.ThriftService, 0)
        fbthriftResults = append(fbthriftResults, metadata.NewThriftService().
    SetName("module.Raiser").
    SetFunctions(
        []*metadata.ThriftFunction{
            metadata.NewThriftFunction().
    SetName("doBland").
    SetIsOneway(false).
    SetReturnType(premadeThriftType_void),
            metadata.NewThriftFunction().
    SetName("doRaise").
    SetIsOneway(false).
    SetReturnType(premadeThriftType_void).
    SetExceptions(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("b").
    SetIsOptional(true).
    SetType(premadeThriftType_module_Banal),
            metadata.NewThriftField().
    SetId(2).
    SetName("f").
    SetIsOptional(true).
    SetType(premadeThriftType_module_Fiery),
            metadata.NewThriftField().
    SetId(3).
    SetName("s").
    SetIsOptional(true).
    SetType(premadeThriftType_module_Serious),
        },
    ),
            metadata.NewThriftFunction().
    SetName("get200").
    SetIsOneway(false).
    SetReturnType(premadeThriftType_string),
            metadata.NewThriftFunction().
    SetName("get500").
    SetIsOneway(false).
    SetReturnType(premadeThriftType_string).
    SetExceptions(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("f").
    SetIsOptional(true).
    SetType(premadeThriftType_module_Fiery),
            metadata.NewThriftField().
    SetId(2).
    SetName("b").
    SetIsOptional(true).
    SetType(premadeThriftType_module_Banal),
            metadata.NewThriftField().
    SetId(3).
    SetName("s").
    SetIsOptional(true).
    SetType(premadeThriftType_module_Serious),
        },
    ),
        },
    ))
        return fbthriftResults
    },
)

// GetMetadataThriftType (INTERNAL USE ONLY).
// Returns metadata ThriftType for a given full type name.
func GetMetadataThriftType(fullName string) *metadata.ThriftType {
    return premadeThriftTypesMapOnce()[fullName]
}

// GetThriftMetadata returns complete Thrift metadata for current and imported packages.
func GetThriftMetadata() *metadata.ThriftMetadata {
    allEnumsMap := make(map[string]*metadata.ThriftEnum)
    allStructsMap := make(map[string]*metadata.ThriftStruct)
    allExceptionsMap := make(map[string]*metadata.ThriftException)
    allServicesMap := make(map[string]*metadata.ThriftService)

    // Add enum metadatas from the current program...
    for _, enumMetadata := range enumMetadatasOnce() {
        allEnumsMap[enumMetadata.GetName()] = enumMetadata
    }
    // Add struct metadatas from the current program...
    for _, structMetadata := range structMetadatasOnce() {
        allStructsMap[structMetadata.GetName()] = structMetadata
    }
    // Add exception metadatas from the current program...
    for _, exceptionMetadata := range exceptionMetadatasOnce() {
        allExceptionsMap[exceptionMetadata.GetName()] = exceptionMetadata
    }
    // Add service metadatas from the current program...
    for _, serviceMetadata := range serviceMetadatasOnce() {
        allServicesMap[serviceMetadata.GetName()] = serviceMetadata
    }

    // Obtain Thrift metadatas from recursively included programs...
    var recursiveThriftMetadatas []*metadata.ThriftMetadata

    // ...now merge metadatas from recursively included programs.
    for _, thriftMetadata := range recursiveThriftMetadatas {
        maps.Copy(allEnumsMap, thriftMetadata.GetEnums())
        maps.Copy(allStructsMap, thriftMetadata.GetStructs())
        maps.Copy(allExceptionsMap, thriftMetadata.GetExceptions())
        maps.Copy(allServicesMap, thriftMetadata.GetServices())
    }

    return metadata.NewThriftMetadata().
        SetEnums(allEnumsMap).
        SetStructs(allStructsMap).
        SetExceptions(allExceptionsMap).
        SetServices(allServicesMap)
}

// GetThriftMetadataForService returns Thrift metadata for the given service.
func GetThriftMetadataForService(scopedServiceName string) *metadata.ThriftMetadata {
    thriftMetadata := GetThriftMetadata()

    allServicesMap := thriftMetadata.GetServices()
    relevantServicesMap := make(map[string]*metadata.ThriftService)

    serviceMetadata := allServicesMap[scopedServiceName]
    // Visit and record all recursive parents of the target service.
    for serviceMetadata != nil {
        relevantServicesMap[serviceMetadata.GetName()] = serviceMetadata
        if serviceMetadata.IsSetParent() {
            serviceMetadata = allServicesMap[serviceMetadata.GetParent()]
        } else {
            serviceMetadata = nil
        }
    }

    thriftMetadata.SetServices(relevantServicesMap)

    return thriftMetadata
}
