// Code generated by protoc-gen-go. DO NOT EDIT.
// versions:
// 	protoc-gen-go v1.28.0
// 	protoc        v3.19.4
// source: interpretability.proto

package v0

import (
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

// This message denotes the LIME interpretability sampling parameters
type LimeSamplingParams struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	// the instance index for explain
	ExplainInstanceIdx int32 `protobuf:"varint,1,opt,name=explain_instance_idx,json=explainInstanceIdx,proto3" json:"explain_instance_idx,omitempty"`
	// whether sampling around the above instance
	SampleAroundInstance bool `protobuf:"varint,2,opt,name=sample_around_instance,json=sampleAroundInstance,proto3" json:"sample_around_instance,omitempty"`
	// number of total samples to be generated
	NumTotalSamples int32 `protobuf:"varint,3,opt,name=num_total_samples,json=numTotalSamples,proto3" json:"num_total_samples,omitempty"`
	// the sampling method, now only support "gaussian"
	SamplingMethod string `protobuf:"bytes,4,opt,name=sampling_method,json=samplingMethod,proto3" json:"sampling_method,omitempty"`
	// generated samples save file
	GeneratedSampleFile string `protobuf:"bytes,5,opt,name=generated_sample_file,json=generatedSampleFile,proto3" json:"generated_sample_file,omitempty"`
}

func (x *LimeSamplingParams) Reset() {
	*x = LimeSamplingParams{}
	if protoimpl.UnsafeEnabled {
		mi := &file_interpretability_proto_msgTypes[0]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *LimeSamplingParams) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*LimeSamplingParams) ProtoMessage() {}

func (x *LimeSamplingParams) ProtoReflect() protoreflect.Message {
	mi := &file_interpretability_proto_msgTypes[0]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use LimeSamplingParams.ProtoReflect.Descriptor instead.
func (*LimeSamplingParams) Descriptor() ([]byte, []int) {
	return file_interpretability_proto_rawDescGZIP(), []int{0}
}

func (x *LimeSamplingParams) GetExplainInstanceIdx() int32 {
	if x != nil {
		return x.ExplainInstanceIdx
	}
	return 0
}

func (x *LimeSamplingParams) GetSampleAroundInstance() bool {
	if x != nil {
		return x.SampleAroundInstance
	}
	return false
}

func (x *LimeSamplingParams) GetNumTotalSamples() int32 {
	if x != nil {
		return x.NumTotalSamples
	}
	return 0
}

func (x *LimeSamplingParams) GetSamplingMethod() string {
	if x != nil {
		return x.SamplingMethod
	}
	return ""
}

func (x *LimeSamplingParams) GetGeneratedSampleFile() string {
	if x != nil {
		return x.GeneratedSampleFile
	}
	return ""
}

// This message denotes the LIME interpretability compute predictions parameters
type LimeCompPredictionParams struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	// vertical original model name
	OriginalModelName string `protobuf:"bytes,1,opt,name=original_model_name,json=originalModelName,proto3" json:"original_model_name,omitempty"`
	// vertical original model saved file
	OriginalModelSavedFile string `protobuf:"bytes,2,opt,name=original_model_saved_file,json=originalModelSavedFile,proto3" json:"original_model_saved_file,omitempty"`
	// generated samples save file
	GeneratedSampleFile string `protobuf:"bytes,3,opt,name=generated_sample_file,json=generatedSampleFile,proto3" json:"generated_sample_file,omitempty"`
	// type of model tasks, 'regression' or 'classification'
	ModelType string `protobuf:"bytes,4,opt,name=model_type,json=modelType,proto3" json:"model_type,omitempty"`
	// number of classes in classification, set to 1 if regression
	ClassNum int32 `protobuf:"varint,5,opt,name=class_num,json=classNum,proto3" json:"class_num,omitempty"`
	// prediction save file
	ComputedPredictionFile string `protobuf:"bytes,6,opt,name=computed_prediction_file,json=computedPredictionFile,proto3" json:"computed_prediction_file,omitempty"`
}

func (x *LimeCompPredictionParams) Reset() {
	*x = LimeCompPredictionParams{}
	if protoimpl.UnsafeEnabled {
		mi := &file_interpretability_proto_msgTypes[1]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *LimeCompPredictionParams) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*LimeCompPredictionParams) ProtoMessage() {}

func (x *LimeCompPredictionParams) ProtoReflect() protoreflect.Message {
	mi := &file_interpretability_proto_msgTypes[1]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use LimeCompPredictionParams.ProtoReflect.Descriptor instead.
func (*LimeCompPredictionParams) Descriptor() ([]byte, []int) {
	return file_interpretability_proto_rawDescGZIP(), []int{1}
}

func (x *LimeCompPredictionParams) GetOriginalModelName() string {
	if x != nil {
		return x.OriginalModelName
	}
	return ""
}

func (x *LimeCompPredictionParams) GetOriginalModelSavedFile() string {
	if x != nil {
		return x.OriginalModelSavedFile
	}
	return ""
}

func (x *LimeCompPredictionParams) GetGeneratedSampleFile() string {
	if x != nil {
		return x.GeneratedSampleFile
	}
	return ""
}

func (x *LimeCompPredictionParams) GetModelType() string {
	if x != nil {
		return x.ModelType
	}
	return ""
}

func (x *LimeCompPredictionParams) GetClassNum() int32 {
	if x != nil {
		return x.ClassNum
	}
	return 0
}

func (x *LimeCompPredictionParams) GetComputedPredictionFile() string {
	if x != nil {
		return x.ComputedPredictionFile
	}
	return ""
}

// This message denotes the compute sample weights parameters
type LimeCompWeightsParams struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	// the instance index for explain
	ExplainInstanceIdx int32 `protobuf:"varint,1,opt,name=explain_instance_idx,json=explainInstanceIdx,proto3" json:"explain_instance_idx,omitempty"`
	// generated samples save file
	GeneratedSampleFile string `protobuf:"bytes,2,opt,name=generated_sample_file,json=generatedSampleFile,proto3" json:"generated_sample_file,omitempty"`
	// prediction save file
	ComputedPredictionFile string `protobuf:"bytes,3,opt,name=computed_prediction_file,json=computedPredictionFile,proto3" json:"computed_prediction_file,omitempty"`
	// whether it is pre-computed
	IsPrecompute bool `protobuf:"varint,4,opt,name=is_precompute,json=isPrecompute,proto3" json:"is_precompute,omitempty"`
	// number of samples to be generated or selected
	NumSamples int32 `protobuf:"varint,5,opt,name=num_samples,json=numSamples,proto3" json:"num_samples,omitempty"`
	// number of classes in classification, set to 1 if regression
	ClassNum int32 `protobuf:"varint,6,opt,name=class_num,json=classNum,proto3" json:"class_num,omitempty"`
	// the metric for computing the distance, only "euclidean"
	DistanceMetric string `protobuf:"bytes,7,opt,name=distance_metric,json=distanceMetric,proto3" json:"distance_metric,omitempty"`
	// kernel, similarity kernel that takes euclidean distances and kernel width
	// as input and outputs weights in (0,1). If not specified, default is exponential kernel
	Kernel string `protobuf:"bytes,8,opt,name=kernel,proto3" json:"kernel,omitempty"`
	// width for the kernel
	KernelWidth float64 `protobuf:"fixed64,9,opt,name=kernel_width,json=kernelWidth,proto3" json:"kernel_width,omitempty"`
	// sample weights file to be saved
	SampleWeightsFile string `protobuf:"bytes,10,opt,name=sample_weights_file,json=sampleWeightsFile,proto3" json:"sample_weights_file,omitempty"`
	// selected samples to be saved if is_precompute = true
	SelectedSamplesFile string `protobuf:"bytes,11,opt,name=selected_samples_file,json=selectedSamplesFile,proto3" json:"selected_samples_file,omitempty"`
	// selected predictions to be saved if is_precompute = true
	SelectedPredictionsFile string `protobuf:"bytes,12,opt,name=selected_predictions_file,json=selectedPredictionsFile,proto3" json:"selected_predictions_file,omitempty"`
}

func (x *LimeCompWeightsParams) Reset() {
	*x = LimeCompWeightsParams{}
	if protoimpl.UnsafeEnabled {
		mi := &file_interpretability_proto_msgTypes[2]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *LimeCompWeightsParams) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*LimeCompWeightsParams) ProtoMessage() {}

func (x *LimeCompWeightsParams) ProtoReflect() protoreflect.Message {
	mi := &file_interpretability_proto_msgTypes[2]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use LimeCompWeightsParams.ProtoReflect.Descriptor instead.
func (*LimeCompWeightsParams) Descriptor() ([]byte, []int) {
	return file_interpretability_proto_rawDescGZIP(), []int{2}
}

func (x *LimeCompWeightsParams) GetExplainInstanceIdx() int32 {
	if x != nil {
		return x.ExplainInstanceIdx
	}
	return 0
}

func (x *LimeCompWeightsParams) GetGeneratedSampleFile() string {
	if x != nil {
		return x.GeneratedSampleFile
	}
	return ""
}

func (x *LimeCompWeightsParams) GetComputedPredictionFile() string {
	if x != nil {
		return x.ComputedPredictionFile
	}
	return ""
}

func (x *LimeCompWeightsParams) GetIsPrecompute() bool {
	if x != nil {
		return x.IsPrecompute
	}
	return false
}

func (x *LimeCompWeightsParams) GetNumSamples() int32 {
	if x != nil {
		return x.NumSamples
	}
	return 0
}

func (x *LimeCompWeightsParams) GetClassNum() int32 {
	if x != nil {
		return x.ClassNum
	}
	return 0
}

func (x *LimeCompWeightsParams) GetDistanceMetric() string {
	if x != nil {
		return x.DistanceMetric
	}
	return ""
}

func (x *LimeCompWeightsParams) GetKernel() string {
	if x != nil {
		return x.Kernel
	}
	return ""
}

func (x *LimeCompWeightsParams) GetKernelWidth() float64 {
	if x != nil {
		return x.KernelWidth
	}
	return 0
}

func (x *LimeCompWeightsParams) GetSampleWeightsFile() string {
	if x != nil {
		return x.SampleWeightsFile
	}
	return ""
}

func (x *LimeCompWeightsParams) GetSelectedSamplesFile() string {
	if x != nil {
		return x.SelectedSamplesFile
	}
	return ""
}

func (x *LimeCompWeightsParams) GetSelectedPredictionsFile() string {
	if x != nil {
		return x.SelectedPredictionsFile
	}
	return ""
}

// This message denotes the LIME feature selection parameters
type LimeFeatSelParams struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	// selected samples file
	SelectedSamplesFile string `protobuf:"bytes,1,opt,name=selected_samples_file,json=selectedSamplesFile,proto3" json:"selected_samples_file,omitempty"`
	// selected predictions file
	SelectedPredictionsFile string `protobuf:"bytes,2,opt,name=selected_predictions_file,json=selectedPredictionsFile,proto3" json:"selected_predictions_file,omitempty"`
	// the sample weights file
	SampleWeightsFile string `protobuf:"bytes,3,opt,name=sample_weights_file,json=sampleWeightsFile,proto3" json:"sample_weights_file,omitempty"`
	// number of samples generated or selected
	NumSamples int32 `protobuf:"varint,4,opt,name=num_samples,json=numSamples,proto3" json:"num_samples,omitempty"`
	// number of classes in classification, set to 1 if regression
	ClassNum int32 `protobuf:"varint,5,opt,name=class_num,json=classNum,proto3" json:"class_num,omitempty"`
	// the label id to be explained
	ClassId int32 `protobuf:"varint,6,opt,name=class_id,json=classId,proto3" json:"class_id,omitempty"`
	// feature selection method, current options are 'pearson', 'lasso_path',
	FeatureSelection string `protobuf:"bytes,7,opt,name=feature_selection,json=featureSelection,proto3" json:"feature_selection,omitempty"`
	// number of features to be explained in the interpret model
	NumExplainedFeatures int32 `protobuf:"varint,8,opt,name=num_explained_features,json=numExplainedFeatures,proto3" json:"num_explained_features,omitempty"`
	// selected features to be saved
	SelectedFeaturesFile string `protobuf:"bytes,9,opt,name=selected_features_file,json=selectedFeaturesFile,proto3" json:"selected_features_file,omitempty"`
}

func (x *LimeFeatSelParams) Reset() {
	*x = LimeFeatSelParams{}
	if protoimpl.UnsafeEnabled {
		mi := &file_interpretability_proto_msgTypes[3]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *LimeFeatSelParams) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*LimeFeatSelParams) ProtoMessage() {}

func (x *LimeFeatSelParams) ProtoReflect() protoreflect.Message {
	mi := &file_interpretability_proto_msgTypes[3]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use LimeFeatSelParams.ProtoReflect.Descriptor instead.
func (*LimeFeatSelParams) Descriptor() ([]byte, []int) {
	return file_interpretability_proto_rawDescGZIP(), []int{3}
}

func (x *LimeFeatSelParams) GetSelectedSamplesFile() string {
	if x != nil {
		return x.SelectedSamplesFile
	}
	return ""
}

func (x *LimeFeatSelParams) GetSelectedPredictionsFile() string {
	if x != nil {
		return x.SelectedPredictionsFile
	}
	return ""
}

func (x *LimeFeatSelParams) GetSampleWeightsFile() string {
	if x != nil {
		return x.SampleWeightsFile
	}
	return ""
}

func (x *LimeFeatSelParams) GetNumSamples() int32 {
	if x != nil {
		return x.NumSamples
	}
	return 0
}

func (x *LimeFeatSelParams) GetClassNum() int32 {
	if x != nil {
		return x.ClassNum
	}
	return 0
}

func (x *LimeFeatSelParams) GetClassId() int32 {
	if x != nil {
		return x.ClassId
	}
	return 0
}

func (x *LimeFeatSelParams) GetFeatureSelection() string {
	if x != nil {
		return x.FeatureSelection
	}
	return ""
}

func (x *LimeFeatSelParams) GetNumExplainedFeatures() int32 {
	if x != nil {
		return x.NumExplainedFeatures
	}
	return 0
}

func (x *LimeFeatSelParams) GetSelectedFeaturesFile() string {
	if x != nil {
		return x.SelectedFeaturesFile
	}
	return ""
}

// This message denotes the LIME interpret model training parameters
type LimeInterpretParams struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	// selected data file, either selected_samples_file or selected_features_file
	SelectedDataFile string `protobuf:"bytes,1,opt,name=selected_data_file,json=selectedDataFile,proto3" json:"selected_data_file,omitempty"`
	// selected predictions saved
	SelectedPredictionsFile string `protobuf:"bytes,2,opt,name=selected_predictions_file,json=selectedPredictionsFile,proto3" json:"selected_predictions_file,omitempty"`
	// sample weights file saved
	SampleWeightsFile string `protobuf:"bytes,3,opt,name=sample_weights_file,json=sampleWeightsFile,proto3" json:"sample_weights_file,omitempty"`
	// number of samples generated or selected
	NumSamples int32 `protobuf:"varint,4,opt,name=num_samples,json=numSamples,proto3" json:"num_samples,omitempty"`
	// number of classes in classification, set to 1 if regression
	ClassNum int32 `protobuf:"varint,5,opt,name=class_num,json=classNum,proto3" json:"class_num,omitempty"`
	// the label id to be explained
	ClassId int32 `protobuf:"varint,6,opt,name=class_id,json=classId,proto3" json:"class_id,omitempty"`
	// interpretable model name, linear_regression or decision_tree
	InterpretModelName string `protobuf:"bytes,7,opt,name=interpret_model_name,json=interpretModelName,proto3" json:"interpret_model_name,omitempty"`
	// interpretable model params, should be serialized LinearRegressionParams or DecisionTreeParams
	InterpretModelParam string `protobuf:"bytes,8,opt,name=interpret_model_param,json=interpretModelParam,proto3" json:"interpret_model_param,omitempty"`
	// explanation report
	ExplanationReport string `protobuf:"bytes,9,opt,name=explanation_report,json=explanationReport,proto3" json:"explanation_report,omitempty"`
}

func (x *LimeInterpretParams) Reset() {
	*x = LimeInterpretParams{}
	if protoimpl.UnsafeEnabled {
		mi := &file_interpretability_proto_msgTypes[4]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *LimeInterpretParams) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*LimeInterpretParams) ProtoMessage() {}

func (x *LimeInterpretParams) ProtoReflect() protoreflect.Message {
	mi := &file_interpretability_proto_msgTypes[4]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use LimeInterpretParams.ProtoReflect.Descriptor instead.
func (*LimeInterpretParams) Descriptor() ([]byte, []int) {
	return file_interpretability_proto_rawDescGZIP(), []int{4}
}

func (x *LimeInterpretParams) GetSelectedDataFile() string {
	if x != nil {
		return x.SelectedDataFile
	}
	return ""
}

func (x *LimeInterpretParams) GetSelectedPredictionsFile() string {
	if x != nil {
		return x.SelectedPredictionsFile
	}
	return ""
}

func (x *LimeInterpretParams) GetSampleWeightsFile() string {
	if x != nil {
		return x.SampleWeightsFile
	}
	return ""
}

func (x *LimeInterpretParams) GetNumSamples() int32 {
	if x != nil {
		return x.NumSamples
	}
	return 0
}

func (x *LimeInterpretParams) GetClassNum() int32 {
	if x != nil {
		return x.ClassNum
	}
	return 0
}

func (x *LimeInterpretParams) GetClassId() int32 {
	if x != nil {
		return x.ClassId
	}
	return 0
}

func (x *LimeInterpretParams) GetInterpretModelName() string {
	if x != nil {
		return x.InterpretModelName
	}
	return ""
}

func (x *LimeInterpretParams) GetInterpretModelParam() string {
	if x != nil {
		return x.InterpretModelParam
	}
	return ""
}

func (x *LimeInterpretParams) GetExplanationReport() string {
	if x != nil {
		return x.ExplanationReport
	}
	return ""
}

var File_interpretability_proto protoreflect.FileDescriptor

var file_interpretability_proto_rawDesc = []byte{
	0x0a, 0x16, 0x69, 0x6e, 0x74, 0x65, 0x72, 0x70, 0x72, 0x65, 0x74, 0x61, 0x62, 0x69, 0x6c, 0x69,
	0x74, 0x79, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x12, 0x19, 0x63, 0x6f, 0x6d, 0x2e, 0x6e, 0x75,
	0x73, 0x2e, 0x64, 0x62, 0x73, 0x79, 0x74, 0x65, 0x6d, 0x2e, 0x66, 0x61, 0x6c, 0x63, 0x6f, 0x6e,
	0x2e, 0x76, 0x30, 0x22, 0x85, 0x02, 0x0a, 0x12, 0x4c, 0x69, 0x6d, 0x65, 0x53, 0x61, 0x6d, 0x70,
	0x6c, 0x69, 0x6e, 0x67, 0x50, 0x61, 0x72, 0x61, 0x6d, 0x73, 0x12, 0x30, 0x0a, 0x14, 0x65, 0x78,
	0x70, 0x6c, 0x61, 0x69, 0x6e, 0x5f, 0x69, 0x6e, 0x73, 0x74, 0x61, 0x6e, 0x63, 0x65, 0x5f, 0x69,
	0x64, 0x78, 0x18, 0x01, 0x20, 0x01, 0x28, 0x05, 0x52, 0x12, 0x65, 0x78, 0x70, 0x6c, 0x61, 0x69,
	0x6e, 0x49, 0x6e, 0x73, 0x74, 0x61, 0x6e, 0x63, 0x65, 0x49, 0x64, 0x78, 0x12, 0x34, 0x0a, 0x16,
	0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x5f, 0x61, 0x72, 0x6f, 0x75, 0x6e, 0x64, 0x5f, 0x69, 0x6e,
	0x73, 0x74, 0x61, 0x6e, 0x63, 0x65, 0x18, 0x02, 0x20, 0x01, 0x28, 0x08, 0x52, 0x14, 0x73, 0x61,
	0x6d, 0x70, 0x6c, 0x65, 0x41, 0x72, 0x6f, 0x75, 0x6e, 0x64, 0x49, 0x6e, 0x73, 0x74, 0x61, 0x6e,
	0x63, 0x65, 0x12, 0x2a, 0x0a, 0x11, 0x6e, 0x75, 0x6d, 0x5f, 0x74, 0x6f, 0x74, 0x61, 0x6c, 0x5f,
	0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x73, 0x18, 0x03, 0x20, 0x01, 0x28, 0x05, 0x52, 0x0f, 0x6e,
	0x75, 0x6d, 0x54, 0x6f, 0x74, 0x61, 0x6c, 0x53, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x73, 0x12, 0x27,
	0x0a, 0x0f, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x69, 0x6e, 0x67, 0x5f, 0x6d, 0x65, 0x74, 0x68, 0x6f,
	0x64, 0x18, 0x04, 0x20, 0x01, 0x28, 0x09, 0x52, 0x0e, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x69, 0x6e,
	0x67, 0x4d, 0x65, 0x74, 0x68, 0x6f, 0x64, 0x12, 0x32, 0x0a, 0x15, 0x67, 0x65, 0x6e, 0x65, 0x72,
	0x61, 0x74, 0x65, 0x64, 0x5f, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x5f, 0x66, 0x69, 0x6c, 0x65,
	0x18, 0x05, 0x20, 0x01, 0x28, 0x09, 0x52, 0x13, 0x67, 0x65, 0x6e, 0x65, 0x72, 0x61, 0x74, 0x65,
	0x64, 0x53, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x46, 0x69, 0x6c, 0x65, 0x22, 0xaf, 0x02, 0x0a, 0x18,
	0x4c, 0x69, 0x6d, 0x65, 0x43, 0x6f, 0x6d, 0x70, 0x50, 0x72, 0x65, 0x64, 0x69, 0x63, 0x74, 0x69,
	0x6f, 0x6e, 0x50, 0x61, 0x72, 0x61, 0x6d, 0x73, 0x12, 0x2e, 0x0a, 0x13, 0x6f, 0x72, 0x69, 0x67,
	0x69, 0x6e, 0x61, 0x6c, 0x5f, 0x6d, 0x6f, 0x64, 0x65, 0x6c, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x18,
	0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x11, 0x6f, 0x72, 0x69, 0x67, 0x69, 0x6e, 0x61, 0x6c, 0x4d,
	0x6f, 0x64, 0x65, 0x6c, 0x4e, 0x61, 0x6d, 0x65, 0x12, 0x39, 0x0a, 0x19, 0x6f, 0x72, 0x69, 0x67,
	0x69, 0x6e, 0x61, 0x6c, 0x5f, 0x6d, 0x6f, 0x64, 0x65, 0x6c, 0x5f, 0x73, 0x61, 0x76, 0x65, 0x64,
	0x5f, 0x66, 0x69, 0x6c, 0x65, 0x18, 0x02, 0x20, 0x01, 0x28, 0x09, 0x52, 0x16, 0x6f, 0x72, 0x69,
	0x67, 0x69, 0x6e, 0x61, 0x6c, 0x4d, 0x6f, 0x64, 0x65, 0x6c, 0x53, 0x61, 0x76, 0x65, 0x64, 0x46,
	0x69, 0x6c, 0x65, 0x12, 0x32, 0x0a, 0x15, 0x67, 0x65, 0x6e, 0x65, 0x72, 0x61, 0x74, 0x65, 0x64,
	0x5f, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x5f, 0x66, 0x69, 0x6c, 0x65, 0x18, 0x03, 0x20, 0x01,
	0x28, 0x09, 0x52, 0x13, 0x67, 0x65, 0x6e, 0x65, 0x72, 0x61, 0x74, 0x65, 0x64, 0x53, 0x61, 0x6d,
	0x70, 0x6c, 0x65, 0x46, 0x69, 0x6c, 0x65, 0x12, 0x1d, 0x0a, 0x0a, 0x6d, 0x6f, 0x64, 0x65, 0x6c,
	0x5f, 0x74, 0x79, 0x70, 0x65, 0x18, 0x04, 0x20, 0x01, 0x28, 0x09, 0x52, 0x09, 0x6d, 0x6f, 0x64,
	0x65, 0x6c, 0x54, 0x79, 0x70, 0x65, 0x12, 0x1b, 0x0a, 0x09, 0x63, 0x6c, 0x61, 0x73, 0x73, 0x5f,
	0x6e, 0x75, 0x6d, 0x18, 0x05, 0x20, 0x01, 0x28, 0x05, 0x52, 0x08, 0x63, 0x6c, 0x61, 0x73, 0x73,
	0x4e, 0x75, 0x6d, 0x12, 0x38, 0x0a, 0x18, 0x63, 0x6f, 0x6d, 0x70, 0x75, 0x74, 0x65, 0x64, 0x5f,
	0x70, 0x72, 0x65, 0x64, 0x69, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x5f, 0x66, 0x69, 0x6c, 0x65, 0x18,
	0x06, 0x20, 0x01, 0x28, 0x09, 0x52, 0x16, 0x63, 0x6f, 0x6d, 0x70, 0x75, 0x74, 0x65, 0x64, 0x50,
	0x72, 0x65, 0x64, 0x69, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x46, 0x69, 0x6c, 0x65, 0x22, 0x9e, 0x04,
	0x0a, 0x15, 0x4c, 0x69, 0x6d, 0x65, 0x43, 0x6f, 0x6d, 0x70, 0x57, 0x65, 0x69, 0x67, 0x68, 0x74,
	0x73, 0x50, 0x61, 0x72, 0x61, 0x6d, 0x73, 0x12, 0x30, 0x0a, 0x14, 0x65, 0x78, 0x70, 0x6c, 0x61,
	0x69, 0x6e, 0x5f, 0x69, 0x6e, 0x73, 0x74, 0x61, 0x6e, 0x63, 0x65, 0x5f, 0x69, 0x64, 0x78, 0x18,
	0x01, 0x20, 0x01, 0x28, 0x05, 0x52, 0x12, 0x65, 0x78, 0x70, 0x6c, 0x61, 0x69, 0x6e, 0x49, 0x6e,
	0x73, 0x74, 0x61, 0x6e, 0x63, 0x65, 0x49, 0x64, 0x78, 0x12, 0x32, 0x0a, 0x15, 0x67, 0x65, 0x6e,
	0x65, 0x72, 0x61, 0x74, 0x65, 0x64, 0x5f, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x5f, 0x66, 0x69,
	0x6c, 0x65, 0x18, 0x02, 0x20, 0x01, 0x28, 0x09, 0x52, 0x13, 0x67, 0x65, 0x6e, 0x65, 0x72, 0x61,
	0x74, 0x65, 0x64, 0x53, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x46, 0x69, 0x6c, 0x65, 0x12, 0x38, 0x0a,
	0x18, 0x63, 0x6f, 0x6d, 0x70, 0x75, 0x74, 0x65, 0x64, 0x5f, 0x70, 0x72, 0x65, 0x64, 0x69, 0x63,
	0x74, 0x69, 0x6f, 0x6e, 0x5f, 0x66, 0x69, 0x6c, 0x65, 0x18, 0x03, 0x20, 0x01, 0x28, 0x09, 0x52,
	0x16, 0x63, 0x6f, 0x6d, 0x70, 0x75, 0x74, 0x65, 0x64, 0x50, 0x72, 0x65, 0x64, 0x69, 0x63, 0x74,
	0x69, 0x6f, 0x6e, 0x46, 0x69, 0x6c, 0x65, 0x12, 0x23, 0x0a, 0x0d, 0x69, 0x73, 0x5f, 0x70, 0x72,
	0x65, 0x63, 0x6f, 0x6d, 0x70, 0x75, 0x74, 0x65, 0x18, 0x04, 0x20, 0x01, 0x28, 0x08, 0x52, 0x0c,
	0x69, 0x73, 0x50, 0x72, 0x65, 0x63, 0x6f, 0x6d, 0x70, 0x75, 0x74, 0x65, 0x12, 0x1f, 0x0a, 0x0b,
	0x6e, 0x75, 0x6d, 0x5f, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x73, 0x18, 0x05, 0x20, 0x01, 0x28,
	0x05, 0x52, 0x0a, 0x6e, 0x75, 0x6d, 0x53, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x73, 0x12, 0x1b, 0x0a,
	0x09, 0x63, 0x6c, 0x61, 0x73, 0x73, 0x5f, 0x6e, 0x75, 0x6d, 0x18, 0x06, 0x20, 0x01, 0x28, 0x05,
	0x52, 0x08, 0x63, 0x6c, 0x61, 0x73, 0x73, 0x4e, 0x75, 0x6d, 0x12, 0x27, 0x0a, 0x0f, 0x64, 0x69,
	0x73, 0x74, 0x61, 0x6e, 0x63, 0x65, 0x5f, 0x6d, 0x65, 0x74, 0x72, 0x69, 0x63, 0x18, 0x07, 0x20,
	0x01, 0x28, 0x09, 0x52, 0x0e, 0x64, 0x69, 0x73, 0x74, 0x61, 0x6e, 0x63, 0x65, 0x4d, 0x65, 0x74,
	0x72, 0x69, 0x63, 0x12, 0x16, 0x0a, 0x06, 0x6b, 0x65, 0x72, 0x6e, 0x65, 0x6c, 0x18, 0x08, 0x20,
	0x01, 0x28, 0x09, 0x52, 0x06, 0x6b, 0x65, 0x72, 0x6e, 0x65, 0x6c, 0x12, 0x21, 0x0a, 0x0c, 0x6b,
	0x65, 0x72, 0x6e, 0x65, 0x6c, 0x5f, 0x77, 0x69, 0x64, 0x74, 0x68, 0x18, 0x09, 0x20, 0x01, 0x28,
	0x01, 0x52, 0x0b, 0x6b, 0x65, 0x72, 0x6e, 0x65, 0x6c, 0x57, 0x69, 0x64, 0x74, 0x68, 0x12, 0x2e,
	0x0a, 0x13, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x5f, 0x77, 0x65, 0x69, 0x67, 0x68, 0x74, 0x73,
	0x5f, 0x66, 0x69, 0x6c, 0x65, 0x18, 0x0a, 0x20, 0x01, 0x28, 0x09, 0x52, 0x11, 0x73, 0x61, 0x6d,
	0x70, 0x6c, 0x65, 0x57, 0x65, 0x69, 0x67, 0x68, 0x74, 0x73, 0x46, 0x69, 0x6c, 0x65, 0x12, 0x32,
	0x0a, 0x15, 0x73, 0x65, 0x6c, 0x65, 0x63, 0x74, 0x65, 0x64, 0x5f, 0x73, 0x61, 0x6d, 0x70, 0x6c,
	0x65, 0x73, 0x5f, 0x66, 0x69, 0x6c, 0x65, 0x18, 0x0b, 0x20, 0x01, 0x28, 0x09, 0x52, 0x13, 0x73,
	0x65, 0x6c, 0x65, 0x63, 0x74, 0x65, 0x64, 0x53, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x73, 0x46, 0x69,
	0x6c, 0x65, 0x12, 0x3a, 0x0a, 0x19, 0x73, 0x65, 0x6c, 0x65, 0x63, 0x74, 0x65, 0x64, 0x5f, 0x70,
	0x72, 0x65, 0x64, 0x69, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x73, 0x5f, 0x66, 0x69, 0x6c, 0x65, 0x18,
	0x0c, 0x20, 0x01, 0x28, 0x09, 0x52, 0x17, 0x73, 0x65, 0x6c, 0x65, 0x63, 0x74, 0x65, 0x64, 0x50,
	0x72, 0x65, 0x64, 0x69, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x73, 0x46, 0x69, 0x6c, 0x65, 0x22, 0xa5,
	0x03, 0x0a, 0x11, 0x4c, 0x69, 0x6d, 0x65, 0x46, 0x65, 0x61, 0x74, 0x53, 0x65, 0x6c, 0x50, 0x61,
	0x72, 0x61, 0x6d, 0x73, 0x12, 0x32, 0x0a, 0x15, 0x73, 0x65, 0x6c, 0x65, 0x63, 0x74, 0x65, 0x64,
	0x5f, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x73, 0x5f, 0x66, 0x69, 0x6c, 0x65, 0x18, 0x01, 0x20,
	0x01, 0x28, 0x09, 0x52, 0x13, 0x73, 0x65, 0x6c, 0x65, 0x63, 0x74, 0x65, 0x64, 0x53, 0x61, 0x6d,
	0x70, 0x6c, 0x65, 0x73, 0x46, 0x69, 0x6c, 0x65, 0x12, 0x3a, 0x0a, 0x19, 0x73, 0x65, 0x6c, 0x65,
	0x63, 0x74, 0x65, 0x64, 0x5f, 0x70, 0x72, 0x65, 0x64, 0x69, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x73,
	0x5f, 0x66, 0x69, 0x6c, 0x65, 0x18, 0x02, 0x20, 0x01, 0x28, 0x09, 0x52, 0x17, 0x73, 0x65, 0x6c,
	0x65, 0x63, 0x74, 0x65, 0x64, 0x50, 0x72, 0x65, 0x64, 0x69, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x73,
	0x46, 0x69, 0x6c, 0x65, 0x12, 0x2e, 0x0a, 0x13, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x5f, 0x77,
	0x65, 0x69, 0x67, 0x68, 0x74, 0x73, 0x5f, 0x66, 0x69, 0x6c, 0x65, 0x18, 0x03, 0x20, 0x01, 0x28,
	0x09, 0x52, 0x11, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x57, 0x65, 0x69, 0x67, 0x68, 0x74, 0x73,
	0x46, 0x69, 0x6c, 0x65, 0x12, 0x1f, 0x0a, 0x0b, 0x6e, 0x75, 0x6d, 0x5f, 0x73, 0x61, 0x6d, 0x70,
	0x6c, 0x65, 0x73, 0x18, 0x04, 0x20, 0x01, 0x28, 0x05, 0x52, 0x0a, 0x6e, 0x75, 0x6d, 0x53, 0x61,
	0x6d, 0x70, 0x6c, 0x65, 0x73, 0x12, 0x1b, 0x0a, 0x09, 0x63, 0x6c, 0x61, 0x73, 0x73, 0x5f, 0x6e,
	0x75, 0x6d, 0x18, 0x05, 0x20, 0x01, 0x28, 0x05, 0x52, 0x08, 0x63, 0x6c, 0x61, 0x73, 0x73, 0x4e,
	0x75, 0x6d, 0x12, 0x19, 0x0a, 0x08, 0x63, 0x6c, 0x61, 0x73, 0x73, 0x5f, 0x69, 0x64, 0x18, 0x06,
	0x20, 0x01, 0x28, 0x05, 0x52, 0x07, 0x63, 0x6c, 0x61, 0x73, 0x73, 0x49, 0x64, 0x12, 0x2b, 0x0a,
	0x11, 0x66, 0x65, 0x61, 0x74, 0x75, 0x72, 0x65, 0x5f, 0x73, 0x65, 0x6c, 0x65, 0x63, 0x74, 0x69,
	0x6f, 0x6e, 0x18, 0x07, 0x20, 0x01, 0x28, 0x09, 0x52, 0x10, 0x66, 0x65, 0x61, 0x74, 0x75, 0x72,
	0x65, 0x53, 0x65, 0x6c, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x12, 0x34, 0x0a, 0x16, 0x6e, 0x75,
	0x6d, 0x5f, 0x65, 0x78, 0x70, 0x6c, 0x61, 0x69, 0x6e, 0x65, 0x64, 0x5f, 0x66, 0x65, 0x61, 0x74,
	0x75, 0x72, 0x65, 0x73, 0x18, 0x08, 0x20, 0x01, 0x28, 0x05, 0x52, 0x14, 0x6e, 0x75, 0x6d, 0x45,
	0x78, 0x70, 0x6c, 0x61, 0x69, 0x6e, 0x65, 0x64, 0x46, 0x65, 0x61, 0x74, 0x75, 0x72, 0x65, 0x73,
	0x12, 0x34, 0x0a, 0x16, 0x73, 0x65, 0x6c, 0x65, 0x63, 0x74, 0x65, 0x64, 0x5f, 0x66, 0x65, 0x61,
	0x74, 0x75, 0x72, 0x65, 0x73, 0x5f, 0x66, 0x69, 0x6c, 0x65, 0x18, 0x09, 0x20, 0x01, 0x28, 0x09,
	0x52, 0x14, 0x73, 0x65, 0x6c, 0x65, 0x63, 0x74, 0x65, 0x64, 0x46, 0x65, 0x61, 0x74, 0x75, 0x72,
	0x65, 0x73, 0x46, 0x69, 0x6c, 0x65, 0x22, 0x9d, 0x03, 0x0a, 0x13, 0x4c, 0x69, 0x6d, 0x65, 0x49,
	0x6e, 0x74, 0x65, 0x72, 0x70, 0x72, 0x65, 0x74, 0x50, 0x61, 0x72, 0x61, 0x6d, 0x73, 0x12, 0x2c,
	0x0a, 0x12, 0x73, 0x65, 0x6c, 0x65, 0x63, 0x74, 0x65, 0x64, 0x5f, 0x64, 0x61, 0x74, 0x61, 0x5f,
	0x66, 0x69, 0x6c, 0x65, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x10, 0x73, 0x65, 0x6c, 0x65,
	0x63, 0x74, 0x65, 0x64, 0x44, 0x61, 0x74, 0x61, 0x46, 0x69, 0x6c, 0x65, 0x12, 0x3a, 0x0a, 0x19,
	0x73, 0x65, 0x6c, 0x65, 0x63, 0x74, 0x65, 0x64, 0x5f, 0x70, 0x72, 0x65, 0x64, 0x69, 0x63, 0x74,
	0x69, 0x6f, 0x6e, 0x73, 0x5f, 0x66, 0x69, 0x6c, 0x65, 0x18, 0x02, 0x20, 0x01, 0x28, 0x09, 0x52,
	0x17, 0x73, 0x65, 0x6c, 0x65, 0x63, 0x74, 0x65, 0x64, 0x50, 0x72, 0x65, 0x64, 0x69, 0x63, 0x74,
	0x69, 0x6f, 0x6e, 0x73, 0x46, 0x69, 0x6c, 0x65, 0x12, 0x2e, 0x0a, 0x13, 0x73, 0x61, 0x6d, 0x70,
	0x6c, 0x65, 0x5f, 0x77, 0x65, 0x69, 0x67, 0x68, 0x74, 0x73, 0x5f, 0x66, 0x69, 0x6c, 0x65, 0x18,
	0x03, 0x20, 0x01, 0x28, 0x09, 0x52, 0x11, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x57, 0x65, 0x69,
	0x67, 0x68, 0x74, 0x73, 0x46, 0x69, 0x6c, 0x65, 0x12, 0x1f, 0x0a, 0x0b, 0x6e, 0x75, 0x6d, 0x5f,
	0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x73, 0x18, 0x04, 0x20, 0x01, 0x28, 0x05, 0x52, 0x0a, 0x6e,
	0x75, 0x6d, 0x53, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x73, 0x12, 0x1b, 0x0a, 0x09, 0x63, 0x6c, 0x61,
	0x73, 0x73, 0x5f, 0x6e, 0x75, 0x6d, 0x18, 0x05, 0x20, 0x01, 0x28, 0x05, 0x52, 0x08, 0x63, 0x6c,
	0x61, 0x73, 0x73, 0x4e, 0x75, 0x6d, 0x12, 0x19, 0x0a, 0x08, 0x63, 0x6c, 0x61, 0x73, 0x73, 0x5f,
	0x69, 0x64, 0x18, 0x06, 0x20, 0x01, 0x28, 0x05, 0x52, 0x07, 0x63, 0x6c, 0x61, 0x73, 0x73, 0x49,
	0x64, 0x12, 0x30, 0x0a, 0x14, 0x69, 0x6e, 0x74, 0x65, 0x72, 0x70, 0x72, 0x65, 0x74, 0x5f, 0x6d,
	0x6f, 0x64, 0x65, 0x6c, 0x5f, 0x6e, 0x61, 0x6d, 0x65, 0x18, 0x07, 0x20, 0x01, 0x28, 0x09, 0x52,
	0x12, 0x69, 0x6e, 0x74, 0x65, 0x72, 0x70, 0x72, 0x65, 0x74, 0x4d, 0x6f, 0x64, 0x65, 0x6c, 0x4e,
	0x61, 0x6d, 0x65, 0x12, 0x32, 0x0a, 0x15, 0x69, 0x6e, 0x74, 0x65, 0x72, 0x70, 0x72, 0x65, 0x74,
	0x5f, 0x6d, 0x6f, 0x64, 0x65, 0x6c, 0x5f, 0x70, 0x61, 0x72, 0x61, 0x6d, 0x18, 0x08, 0x20, 0x01,
	0x28, 0x09, 0x52, 0x13, 0x69, 0x6e, 0x74, 0x65, 0x72, 0x70, 0x72, 0x65, 0x74, 0x4d, 0x6f, 0x64,
	0x65, 0x6c, 0x50, 0x61, 0x72, 0x61, 0x6d, 0x12, 0x2d, 0x0a, 0x12, 0x65, 0x78, 0x70, 0x6c, 0x61,
	0x6e, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x5f, 0x72, 0x65, 0x70, 0x6f, 0x72, 0x74, 0x18, 0x09, 0x20,
	0x01, 0x28, 0x09, 0x52, 0x11, 0x65, 0x78, 0x70, 0x6c, 0x61, 0x6e, 0x61, 0x74, 0x69, 0x6f, 0x6e,
	0x52, 0x65, 0x70, 0x6f, 0x72, 0x74, 0x42, 0x05, 0x5a, 0x03, 0x2f, 0x76, 0x30, 0x62, 0x06, 0x70,
	0x72, 0x6f, 0x74, 0x6f, 0x33,
}

var (
	file_interpretability_proto_rawDescOnce sync.Once
	file_interpretability_proto_rawDescData = file_interpretability_proto_rawDesc
)

func file_interpretability_proto_rawDescGZIP() []byte {
	file_interpretability_proto_rawDescOnce.Do(func() {
		file_interpretability_proto_rawDescData = protoimpl.X.CompressGZIP(file_interpretability_proto_rawDescData)
	})
	return file_interpretability_proto_rawDescData
}

var file_interpretability_proto_msgTypes = make([]protoimpl.MessageInfo, 5)
var file_interpretability_proto_goTypes = []interface{}{
	(*LimeSamplingParams)(nil),       // 0: com.nus.dbsytem.falcon.v0.LimeSamplingParams
	(*LimeCompPredictionParams)(nil), // 1: com.nus.dbsytem.falcon.v0.LimeCompPredictionParams
	(*LimeCompWeightsParams)(nil),    // 2: com.nus.dbsytem.falcon.v0.LimeCompWeightsParams
	(*LimeFeatSelParams)(nil),        // 3: com.nus.dbsytem.falcon.v0.LimeFeatSelParams
	(*LimeInterpretParams)(nil),      // 4: com.nus.dbsytem.falcon.v0.LimeInterpretParams
}
var file_interpretability_proto_depIdxs = []int32{
	0, // [0:0] is the sub-list for method output_type
	0, // [0:0] is the sub-list for method input_type
	0, // [0:0] is the sub-list for extension type_name
	0, // [0:0] is the sub-list for extension extendee
	0, // [0:0] is the sub-list for field type_name
}

func init() { file_interpretability_proto_init() }
func file_interpretability_proto_init() {
	if File_interpretability_proto != nil {
		return
	}
	if !protoimpl.UnsafeEnabled {
		file_interpretability_proto_msgTypes[0].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*LimeSamplingParams); i {
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
		file_interpretability_proto_msgTypes[1].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*LimeCompPredictionParams); i {
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
		file_interpretability_proto_msgTypes[2].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*LimeCompWeightsParams); i {
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
		file_interpretability_proto_msgTypes[3].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*LimeFeatSelParams); i {
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
		file_interpretability_proto_msgTypes[4].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*LimeInterpretParams); i {
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
			RawDescriptor: file_interpretability_proto_rawDesc,
			NumEnums:      0,
			NumMessages:   5,
			NumExtensions: 0,
			NumServices:   0,
		},
		GoTypes:           file_interpretability_proto_goTypes,
		DependencyIndexes: file_interpretability_proto_depIdxs,
		MessageInfos:      file_interpretability_proto_msgTypes,
	}.Build()
	File_interpretability_proto = out.File
	file_interpretability_proto_rawDesc = nil
	file_interpretability_proto_goTypes = nil
	file_interpretability_proto_depIdxs = nil
}
