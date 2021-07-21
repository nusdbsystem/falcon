// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: lr.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_lr_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_lr_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3014000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3014000 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
#include "common.pb.h"
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_lr_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_lr_2eproto {
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTableField entries[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::AuxiliaryParseTableField aux[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTable schema[1]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::FieldMetadata field_metadata[];
  static const ::PROTOBUF_NAMESPACE_ID::internal::SerializationTable serialization_table[];
  static const ::PROTOBUF_NAMESPACE_ID::uint32 offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_lr_2eproto;
namespace com {
namespace nus {
namespace dbsytem {
namespace falcon {
namespace v0 {
class LrModel;
class LrModelDefaultTypeInternal;
extern LrModelDefaultTypeInternal _LrModel_default_instance_;
}  // namespace v0
}  // namespace falcon
}  // namespace dbsytem
}  // namespace nus
}  // namespace com
PROTOBUF_NAMESPACE_OPEN
template<> ::com::nus::dbsytem::falcon::v0::LrModel* Arena::CreateMaybeMessage<::com::nus::dbsytem::falcon::v0::LrModel>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace com {
namespace nus {
namespace dbsytem {
namespace falcon {
namespace v0 {

// ===================================================================

class LrModel PROTOBUF_FINAL :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:com.nus.dbsytem.falcon.v0.LrModel) */ {
 public:
  inline LrModel() : LrModel(nullptr) {}
  virtual ~LrModel();

  LrModel(const LrModel& from);
  LrModel(LrModel&& from) noexcept
    : LrModel() {
    *this = ::std::move(from);
  }

  inline LrModel& operator=(const LrModel& from) {
    CopyFrom(from);
    return *this;
  }
  inline LrModel& operator=(LrModel&& from) noexcept {
    if (GetArena() == from.GetArena()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return GetMetadataStatic().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return GetMetadataStatic().reflection;
  }
  static const LrModel& default_instance();

  static inline const LrModel* internal_default_instance() {
    return reinterpret_cast<const LrModel*>(
               &_LrModel_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(LrModel& a, LrModel& b) {
    a.Swap(&b);
  }
  inline void Swap(LrModel* other) {
    if (other == this) return;
    if (GetArena() == other->GetArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(LrModel* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline LrModel* New() const final {
    return CreateMaybeMessage<LrModel>(nullptr);
  }

  LrModel* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<LrModel>(arena);
  }
  void CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void CopyFrom(const LrModel& from);
  void MergeFrom(const LrModel& from);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::PROTOBUF_NAMESPACE_ID::uint8* _InternalSerialize(
      ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  inline void SharedCtor();
  inline void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(LrModel* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "com.nus.dbsytem.falcon.v0.LrModel";
  }
  protected:
  explicit LrModel(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;
  private:
  static ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadataStatic() {
    ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&::descriptor_table_lr_2eproto);
    return ::descriptor_table_lr_2eproto.file_level_metadata[kIndexInFileMessages];
  }

  public:

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kModelWeightsFieldNumber = 2,
    kWeightSizeFieldNumber = 1,
  };
  // repeated .com.nus.dbsytem.falcon.v0.FixedPointEncodedNumber model_weights = 2;
  int model_weights_size() const;
  private:
  int _internal_model_weights_size() const;
  public:
  void clear_model_weights();
  ::com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber* mutable_model_weights(int index);
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber >*
      mutable_model_weights();
  private:
  const ::com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber& _internal_model_weights(int index) const;
  ::com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber* _internal_add_model_weights();
  public:
  const ::com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber& model_weights(int index) const;
  ::com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber* add_model_weights();
  const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber >&
      model_weights() const;

  // int32 weight_size = 1;
  void clear_weight_size();
  ::PROTOBUF_NAMESPACE_ID::int32 weight_size() const;
  void set_weight_size(::PROTOBUF_NAMESPACE_ID::int32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::int32 _internal_weight_size() const;
  void _internal_set_weight_size(::PROTOBUF_NAMESPACE_ID::int32 value);
  public:

  // @@protoc_insertion_point(class_scope:com.nus.dbsytem.falcon.v0.LrModel)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber > model_weights_;
  ::PROTOBUF_NAMESPACE_ID::int32 weight_size_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_lr_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// LrModel

// int32 weight_size = 1;
inline void LrModel::clear_weight_size() {
  weight_size_ = 0;
}
inline ::PROTOBUF_NAMESPACE_ID::int32 LrModel::_internal_weight_size() const {
  return weight_size_;
}
inline ::PROTOBUF_NAMESPACE_ID::int32 LrModel::weight_size() const {
  // @@protoc_insertion_point(field_get:com.nus.dbsytem.falcon.v0.LrModel.weight_size)
  return _internal_weight_size();
}
inline void LrModel::_internal_set_weight_size(::PROTOBUF_NAMESPACE_ID::int32 value) {
  
  weight_size_ = value;
}
inline void LrModel::set_weight_size(::PROTOBUF_NAMESPACE_ID::int32 value) {
  _internal_set_weight_size(value);
  // @@protoc_insertion_point(field_set:com.nus.dbsytem.falcon.v0.LrModel.weight_size)
}

// repeated .com.nus.dbsytem.falcon.v0.FixedPointEncodedNumber model_weights = 2;
inline int LrModel::_internal_model_weights_size() const {
  return model_weights_.size();
}
inline int LrModel::model_weights_size() const {
  return _internal_model_weights_size();
}
inline ::com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber* LrModel::mutable_model_weights(int index) {
  // @@protoc_insertion_point(field_mutable:com.nus.dbsytem.falcon.v0.LrModel.model_weights)
  return model_weights_.Mutable(index);
}
inline ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber >*
LrModel::mutable_model_weights() {
  // @@protoc_insertion_point(field_mutable_list:com.nus.dbsytem.falcon.v0.LrModel.model_weights)
  return &model_weights_;
}
inline const ::com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber& LrModel::_internal_model_weights(int index) const {
  return model_weights_.Get(index);
}
inline const ::com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber& LrModel::model_weights(int index) const {
  // @@protoc_insertion_point(field_get:com.nus.dbsytem.falcon.v0.LrModel.model_weights)
  return _internal_model_weights(index);
}
inline ::com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber* LrModel::_internal_add_model_weights() {
  return model_weights_.Add();
}
inline ::com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber* LrModel::add_model_weights() {
  // @@protoc_insertion_point(field_add:com.nus.dbsytem.falcon.v0.LrModel.model_weights)
  return _internal_add_model_weights();
}
inline const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::com::nus::dbsytem::falcon::v0::FixedPointEncodedNumber >&
LrModel::model_weights() const {
  // @@protoc_insertion_point(field_list:com.nus.dbsytem.falcon.v0.LrModel.model_weights)
  return model_weights_;
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace v0
}  // namespace falcon
}  // namespace dbsytem
}  // namespace nus
}  // namespace com

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_lr_2eproto
