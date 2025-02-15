// Autogenerated by Thrift for thrift/compiler/test/fixtures/default_values/src/module.thrift
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
    premadeThriftType_i32 *metadata.ThriftType = nil
    premadeThriftType_module_TrivialStruct *metadata.ThriftType = nil
    premadeThriftType_module_StructWithNoCustomDefaultValues *metadata.ThriftType = nil
    premadeThriftType_module_StructWithCustomDefaultValues *metadata.ThriftType = nil
)

// Premade Thrift type initializer
var premadeThriftTypesInitOnce = sync.OnceFunc(func() {
    premadeThriftType_i32 = metadata.NewThriftType().SetTPrimitive(
        metadata.ThriftPrimitiveType_THRIFT_I32_TYPE.Ptr(),
    )
    premadeThriftType_module_TrivialStruct = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.TrivialStruct"),
    )
    premadeThriftType_module_StructWithNoCustomDefaultValues = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.StructWithNoCustomDefaultValues"),
    )
    premadeThriftType_module_StructWithCustomDefaultValues = metadata.NewThriftType().SetTStruct(
        metadata.NewThriftStructType().
            SetName("module.StructWithCustomDefaultValues"),
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
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "i32", premadeThriftType_i32 })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "module.TrivialStruct", premadeThriftType_module_TrivialStruct })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "module.StructWithNoCustomDefaultValues", premadeThriftType_module_StructWithNoCustomDefaultValues })
        thriftTypesWithFullName = append(thriftTypesWithFullName, thriftTypeWithFullName{ "module.StructWithCustomDefaultValues", premadeThriftType_module_StructWithCustomDefaultValues })

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
        fbthriftResults = append(fbthriftResults, metadata.NewThriftStruct().
    SetName("module.TrivialStruct").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("int_value").
    SetIsOptional(false).
    SetType(premadeThriftType_i32),
        },
    ))
        fbthriftResults = append(fbthriftResults, metadata.NewThriftStruct().
    SetName("module.StructWithNoCustomDefaultValues").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("unqualified_integer").
    SetIsOptional(false).
    SetType(premadeThriftType_i32),
            metadata.NewThriftField().
    SetId(2).
    SetName("optional_integer").
    SetIsOptional(true).
    SetType(premadeThriftType_i32),
            metadata.NewThriftField().
    SetId(3).
    SetName("required_integer").
    SetIsOptional(false).
    SetType(premadeThriftType_i32),
            metadata.NewThriftField().
    SetId(4).
    SetName("unqualified_struct").
    SetIsOptional(false).
    SetType(premadeThriftType_module_TrivialStruct),
            metadata.NewThriftField().
    SetId(5).
    SetName("optional_struct").
    SetIsOptional(true).
    SetType(premadeThriftType_module_TrivialStruct),
            metadata.NewThriftField().
    SetId(6).
    SetName("required_struct").
    SetIsOptional(false).
    SetType(premadeThriftType_module_TrivialStruct),
        },
    ))
        fbthriftResults = append(fbthriftResults, metadata.NewThriftStruct().
    SetName("module.StructWithCustomDefaultValues").
    SetIsUnion(false).
    SetFields(
        []*metadata.ThriftField{
            metadata.NewThriftField().
    SetId(1).
    SetName("unqualified_integer").
    SetIsOptional(false).
    SetType(premadeThriftType_i32),
            metadata.NewThriftField().
    SetId(2).
    SetName("optional_integer").
    SetIsOptional(true).
    SetType(premadeThriftType_i32),
            metadata.NewThriftField().
    SetId(3).
    SetName("required_integer").
    SetIsOptional(false).
    SetType(premadeThriftType_i32),
            metadata.NewThriftField().
    SetId(4).
    SetName("unqualified_struct").
    SetIsOptional(false).
    SetType(premadeThriftType_module_TrivialStruct),
            metadata.NewThriftField().
    SetId(5).
    SetName("optional_struct").
    SetIsOptional(true).
    SetType(premadeThriftType_module_TrivialStruct),
            metadata.NewThriftField().
    SetId(6).
    SetName("required_struct").
    SetIsOptional(false).
    SetType(premadeThriftType_module_TrivialStruct),
        },
    ))
        return fbthriftResults
    },
)

var exceptionMetadatasOnce = sync.OnceValue(
    func() []*metadata.ThriftException {
        // Relies on premade Thrift types initialization
        premadeThriftTypesInitOnce()

        fbthriftResults := make([]*metadata.ThriftException, 0)
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
