// Code generated by protoc-gen-go. DO NOT EDIT.
// versions:
// 	protoc-gen-go v1.25.0
// 	protoc        v3.15.8
// source: ps_network.proto

package v0

import (
	proto "github.com/golang/protobuf/proto"
	protoreflect "google.golang.org/protobuf/reflect/protoreflect"
	protoimpl "google.golang.org/protobuf/runtime/protoimpl"
	reflect "reflect"
	sync "sync"
)

const (
	// Verify that this generated code is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(20 - protoimpl.MinVersion)
	// Verify that runtime/protoimpl is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(protoimpl.MaxVersion - 20)
)

// This is a compile-time assertion that a sufficiently up-to-date version
// of the legacy proto package is being used.
const _ = proto.ProtoPackageIsVersion4

// one ps and many workers,
type PSNetworkConfig struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	// an array of followers
	Workers []*Worker `protobuf:"bytes,1,rep,name=workers,proto3" json:"workers,omitempty"`
	// ps information, ps need multiple port where each is corresponding to one worker
	Ps []*PS `protobuf:"bytes,2,rep,name=ps,proto3" json:"ps,omitempty"`
}

func (x *PSNetworkConfig) Reset() {
	*x = PSNetworkConfig{}
	if protoimpl.UnsafeEnabled {
		mi := &file_ps_network_proto_msgTypes[0]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *PSNetworkConfig) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*PSNetworkConfig) ProtoMessage() {}

func (x *PSNetworkConfig) ProtoReflect() protoreflect.Message {
	mi := &file_ps_network_proto_msgTypes[0]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use PSNetworkConfig.ProtoReflect.Descriptor instead.
func (*PSNetworkConfig) Descriptor() ([]byte, []int) {
	return file_ps_network_proto_rawDescGZIP(), []int{0}
}

func (x *PSNetworkConfig) GetWorkers() []*Worker {
	if x != nil {
		return x.Workers
	}
	return nil
}

func (x *PSNetworkConfig) GetPs() []*PS {
	if x != nil {
		return x.Ps
	}
	return nil
}

// ps will read those and send message to the worker address
type Worker struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	// ip of follower
	WorkerIp string `protobuf:"bytes,1,opt,name=worker_ip,json=workerIp,proto3" json:"worker_ip,omitempty"`
	// port of follower
	WorkerPort int32 `protobuf:"varint,2,opt,name=worker_port,json=workerPort,proto3" json:"worker_port,omitempty"`
}

func (x *Worker) Reset() {
	*x = Worker{}
	if protoimpl.UnsafeEnabled {
		mi := &file_ps_network_proto_msgTypes[1]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *Worker) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*Worker) ProtoMessage() {}

func (x *Worker) ProtoReflect() protoreflect.Message {
	mi := &file_ps_network_proto_msgTypes[1]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use Worker.ProtoReflect.Descriptor instead.
func (*Worker) Descriptor() ([]byte, []int) {
	return file_ps_network_proto_rawDescGZIP(), []int{1}
}

func (x *Worker) GetWorkerIp() string {
	if x != nil {
		return x.WorkerIp
	}
	return ""
}

func (x *Worker) GetWorkerPort() int32 {
	if x != nil {
		return x.WorkerPort
	}
	return 0
}

// other workers will send requests to this address
type PS struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	// ip of parameter server
	PsIp string `protobuf:"bytes,1,opt,name=ps_ip,json=psIp,proto3" json:"ps_ip,omitempty"`
	// port of parameter server
	PsPort int32 `protobuf:"varint,2,opt,name=ps_port,json=psPort,proto3" json:"ps_port,omitempty"`
}

func (x *PS) Reset() {
	*x = PS{}
	if protoimpl.UnsafeEnabled {
		mi := &file_ps_network_proto_msgTypes[2]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *PS) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*PS) ProtoMessage() {}

func (x *PS) ProtoReflect() protoreflect.Message {
	mi := &file_ps_network_proto_msgTypes[2]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use PS.ProtoReflect.Descriptor instead.
func (*PS) Descriptor() ([]byte, []int) {
	return file_ps_network_proto_rawDescGZIP(), []int{2}
}

func (x *PS) GetPsIp() string {
	if x != nil {
		return x.PsIp
	}
	return ""
}

func (x *PS) GetPsPort() int32 {
	if x != nil {
		return x.PsPort
	}
	return 0
}

var File_ps_network_proto protoreflect.FileDescriptor

var file_ps_network_proto_rawDesc = []byte{
	0x0a, 0x10, 0x70, 0x73, 0x5f, 0x6e, 0x65, 0x74, 0x77, 0x6f, 0x72, 0x6b, 0x2e, 0x70, 0x72, 0x6f,
	0x74, 0x6f, 0x12, 0x19, 0x63, 0x6f, 0x6d, 0x2e, 0x6e, 0x75, 0x73, 0x2e, 0x64, 0x62, 0x73, 0x79,
	0x74, 0x65, 0x6d, 0x2e, 0x66, 0x61, 0x6c, 0x63, 0x6f, 0x6e, 0x2e, 0x76, 0x30, 0x22, 0x7d, 0x0a,
	0x0f, 0x50, 0x53, 0x4e, 0x65, 0x74, 0x77, 0x6f, 0x72, 0x6b, 0x43, 0x6f, 0x6e, 0x66, 0x69, 0x67,
	0x12, 0x3b, 0x0a, 0x07, 0x77, 0x6f, 0x72, 0x6b, 0x65, 0x72, 0x73, 0x18, 0x01, 0x20, 0x03, 0x28,
	0x0b, 0x32, 0x21, 0x2e, 0x63, 0x6f, 0x6d, 0x2e, 0x6e, 0x75, 0x73, 0x2e, 0x64, 0x62, 0x73, 0x79,
	0x74, 0x65, 0x6d, 0x2e, 0x66, 0x61, 0x6c, 0x63, 0x6f, 0x6e, 0x2e, 0x76, 0x30, 0x2e, 0x57, 0x6f,
	0x72, 0x6b, 0x65, 0x72, 0x52, 0x07, 0x77, 0x6f, 0x72, 0x6b, 0x65, 0x72, 0x73, 0x12, 0x2d, 0x0a,
	0x02, 0x70, 0x73, 0x18, 0x02, 0x20, 0x03, 0x28, 0x0b, 0x32, 0x1d, 0x2e, 0x63, 0x6f, 0x6d, 0x2e,
	0x6e, 0x75, 0x73, 0x2e, 0x64, 0x62, 0x73, 0x79, 0x74, 0x65, 0x6d, 0x2e, 0x66, 0x61, 0x6c, 0x63,
	0x6f, 0x6e, 0x2e, 0x76, 0x30, 0x2e, 0x50, 0x53, 0x52, 0x02, 0x70, 0x73, 0x22, 0x46, 0x0a, 0x06,
	0x57, 0x6f, 0x72, 0x6b, 0x65, 0x72, 0x12, 0x1b, 0x0a, 0x09, 0x77, 0x6f, 0x72, 0x6b, 0x65, 0x72,
	0x5f, 0x69, 0x70, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x08, 0x77, 0x6f, 0x72, 0x6b, 0x65,
	0x72, 0x49, 0x70, 0x12, 0x1f, 0x0a, 0x0b, 0x77, 0x6f, 0x72, 0x6b, 0x65, 0x72, 0x5f, 0x70, 0x6f,
	0x72, 0x74, 0x18, 0x02, 0x20, 0x01, 0x28, 0x05, 0x52, 0x0a, 0x77, 0x6f, 0x72, 0x6b, 0x65, 0x72,
	0x50, 0x6f, 0x72, 0x74, 0x22, 0x32, 0x0a, 0x02, 0x50, 0x53, 0x12, 0x13, 0x0a, 0x05, 0x70, 0x73,
	0x5f, 0x69, 0x70, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x04, 0x70, 0x73, 0x49, 0x70, 0x12,
	0x17, 0x0a, 0x07, 0x70, 0x73, 0x5f, 0x70, 0x6f, 0x72, 0x74, 0x18, 0x02, 0x20, 0x01, 0x28, 0x05,
	0x52, 0x06, 0x70, 0x73, 0x50, 0x6f, 0x72, 0x74, 0x42, 0x05, 0x5a, 0x03, 0x2f, 0x76, 0x30, 0x62,
	0x06, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x33,
}

var (
	file_ps_network_proto_rawDescOnce sync.Once
	file_ps_network_proto_rawDescData = file_ps_network_proto_rawDesc
)

func file_ps_network_proto_rawDescGZIP() []byte {
	file_ps_network_proto_rawDescOnce.Do(func() {
		file_ps_network_proto_rawDescData = protoimpl.X.CompressGZIP(file_ps_network_proto_rawDescData)
	})
	return file_ps_network_proto_rawDescData
}

var file_ps_network_proto_msgTypes = make([]protoimpl.MessageInfo, 3)
var file_ps_network_proto_goTypes = []interface{}{
	(*PSNetworkConfig)(nil), // 0: com.nus.dbsytem.falcon.v0.PSNetworkConfig
	(*Worker)(nil),          // 1: com.nus.dbsytem.falcon.v0.Worker
	(*PS)(nil),              // 2: com.nus.dbsytem.falcon.v0.PS
}
var file_ps_network_proto_depIdxs = []int32{
	1, // 0: com.nus.dbsytem.falcon.v0.PSNetworkConfig.workers:type_name -> com.nus.dbsytem.falcon.v0.Worker
	2, // 1: com.nus.dbsytem.falcon.v0.PSNetworkConfig.ps:type_name -> com.nus.dbsytem.falcon.v0.PS
	2, // [2:2] is the sub-list for method output_type
	2, // [2:2] is the sub-list for method input_type
	2, // [2:2] is the sub-list for extension type_name
	2, // [2:2] is the sub-list for extension extendee
	0, // [0:2] is the sub-list for field type_name
}

func init() { file_ps_network_proto_init() }
func file_ps_network_proto_init() {
	if File_ps_network_proto != nil {
		return
	}
	if !protoimpl.UnsafeEnabled {
		file_ps_network_proto_msgTypes[0].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*PSNetworkConfig); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_ps_network_proto_msgTypes[1].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*Worker); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_ps_network_proto_msgTypes[2].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*PS); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
	}
	type x struct{}
	out := protoimpl.TypeBuilder{
		File: protoimpl.DescBuilder{
			GoPackagePath: reflect.TypeOf(x{}).PkgPath(),
			RawDescriptor: file_ps_network_proto_rawDesc,
			NumEnums:      0,
			NumMessages:   3,
			NumExtensions: 0,
			NumServices:   0,
		},
		GoTypes:           file_ps_network_proto_goTypes,
		DependencyIndexes: file_ps_network_proto_depIdxs,
		MessageInfos:      file_ps_network_proto_msgTypes,
	}.Build()
	File_ps_network_proto = out.File
	file_ps_network_proto_rawDesc = nil
	file_ps_network_proto_goTypes = nil
	file_ps_network_proto_depIdxs = nil
}
