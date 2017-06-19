#ifndef VOXBLOX_CORE_IO_LAYER_IO_H_
#define VOXBLOX_CORE_IO_LAYER_IO_H_

#include <string>

#include <glog/logging.h>

#include "./Block.pb.h"
#include "./Layer.pb.h"
#include "./MapHeader.pb.h"
#include "./Volume.pb.h"
#include "voxblox/core/common.h"
#include "voxblox/core/layer.h"
#include "voxblox/utils/protobuf_utils.h"

namespace voxblox {
namespace io {

template <typename VoxelType>
bool LoadLayer(const std::string& file_path,
               typename Layer<VoxelType>::Ptr* layer_ptr) {
  CHECK_NOTNULL(layer_ptr);

  // Open and check the file
  std::fstream proto_file;
  proto_file.open(file_path, std::fstream::in);
  if (!proto_file.is_open()) {
    LOG(ERROR) << "Could not open protobuf file to load layer: " << file_path;
    return false;
  }

  // Unused byte offset result.
  uint32_t tmp_byte_offset;

  // Get number of messages
  uint32_t num_protos;
  if (!utils::readProtoMsgCountToStream(&proto_file, &num_protos,
                                        &tmp_byte_offset)) {
    LOG(ERROR) << "Could not read number of messages.";
    return false;
  }

  if (num_protos == 0u) {
    LOG(WARNING) << "Empty protobuf file!";
    return false;
  }

  // Get header and create the layer if compatible
  GenericLayerProto<VoxelType> layer_proto;
  if (!utils::readProtoMsgFromStream(&proto_file, &layer_proto,
                                     &tmp_byte_offset)) {
    LOG(ERROR) << "Could not read layer protobuf message.";
    return false;
  }
  *layer_ptr = aligned_shared<Layer<VoxelType> >(layer_proto);
  CHECK(*layer_ptr);

  // Read all blocks and add them to the layer.
  const size_t num_blocks = num_protos - 1;
  for (uint32_t block_idx = 0u; block_idx < num_blocks; ++block_idx) {
    GenericBlockProto<VoxelType> block_proto;
    if (!utils::readProtoMsgFromStream(&proto_file, &block_proto,
                                       &tmp_byte_offset)) {
      LOG(ERROR) << "Could not read block protobuf message number "
                 << block_idx;
      return false;
    }

    if (!(*layer_ptr)->addBlockFromProto(
             block_proto, Layer<VoxelType>::BlockMergingStrategy::kProhibit)) {
      LOG(ERROR) << "Could not add the block protobuf message to the layer!";
      return false;
    }
  }

  return true;
}

/*TODO(mereweth@jpl.nasa.gov) - best name for this function?
 * This function may be useful when debugging a malformed protobuf dump
 * Also prevents segfaulting Python using file_path EsdfMap constructor
 */
template <typename VoxelType>
typename Layer<VoxelType>::Ptr LoadOrCreateLayerHeader(
                                                const std::string& file_path,
                                                FloatingPoint voxel_size,
                                                size_t voxels_per_side) {

  bool success = true;

  // Open and check the file
  std::fstream proto_file;
  proto_file.open(file_path, std::fstream::in);
  if (!success ||
      !proto_file.is_open()) {
    LOG(ERROR) << "Could not open protobuf file to load layer: " << file_path;
    success = false;
  }

  // Unused byte offset result.
  uint32_t tmp_byte_offset;

  // Get number of messages
  uint32_t num_protos;
  if (!success ||
      !utils::readProtoMsgCountToStream(&proto_file, &num_protos,
                                        &tmp_byte_offset)) {
    LOG(ERROR) << "Could not read number of messages.";
    success = false;
  }

  if (!success ||
      (num_protos == 0u)) {
    LOG(WARNING) << "Empty protobuf file!";
    success = false;
  }

  // Get header and create the layer if compatible
  GenericLayerProto<VoxelType> layer_proto;
  if (!success ||
      !utils::readProtoMsgFromStream(&proto_file, &layer_proto,
                                     &tmp_byte_offset)) {
    LOG(ERROR) << "Could not read layer protobuf message.";
    success = false;
  }

  typename Layer<VoxelType>::Ptr layer_ptr;
  if (success) {
    layer_ptr = aligned_shared<Layer<VoxelType> >(layer_proto);
  }
  else {
    layer_ptr = std::make_shared<Layer<VoxelType> >(voxel_size, voxels_per_side);
  }
  CHECK(layer_ptr);

  return layer_ptr;
}

template <typename VoxelType>
bool LoadBlocksFromFile(
    const std::string& file_path,
    typename Layer<VoxelType>::BlockMergingStrategy strategy,
    Layer<VoxelType>* layer_ptr) {
  CHECK_NOTNULL(layer_ptr);

  // Open and check the file
  std::fstream proto_file;
  proto_file.open(file_path, std::fstream::in);
  if (!proto_file.is_open()) {
    LOG(ERROR) << "Could not open protobuf file to load layer: " << file_path;
    return false;
  }

  // Unused byte offset result.
  uint32_t tmp_byte_offset;

  // Get number of messages
  uint32_t num_protos;
  if (!utils::readProtoMsgCountToStream(&proto_file, &num_protos,
                                        &tmp_byte_offset)) {
    LOG(ERROR) << "Could not read number of messages.";
    return false;
  }

  if (num_protos == 0u) {
    LOG(WARNING) << "Empty protobuf file!";
    return false;
  }

  // Get header and check if it is compatible with existing layer.
  GenericLayerProto<VoxelType> layer_proto;
  if (!utils::readProtoMsgFromStream(&proto_file, &layer_proto,
                                     &tmp_byte_offset)) {
    LOG(ERROR) << "Could not read layer protobuf message.";
    return false;
  }
  if (!layer_ptr->isCompatible(layer_proto)) {
    LOG(ERROR) << "The layer information read from file is not compatible with "
                  "the current layer!";
    return false;
  }

  // TODO(mereweth@jpl.nasa.gov) - remove debug
  LOG(WARNING) << (num_protos - 1) << " blocks";

  // Read all blocks and add them to the layer.
  const size_t num_blocks = num_protos - 1;
  for (uint32_t block_idx = 0u; block_idx < num_blocks; ++block_idx) {
    GenericBlockProto<VoxelType> block_proto;
    if (!utils::readProtoMsgFromStream(&proto_file, &block_proto,
                                       &tmp_byte_offset)) {
      LOG(ERROR) << "Could not read block protobuf message number "
                 << block_idx;
      return false;
    }

    if (!layer_ptr->addBlockFromProto(block_proto, strategy)) {
      LOG(ERROR) << "Could not add the block protobuf message to the layer!";
      return false;
    }
  }
  return true;
}

template <typename VoxelType>
bool SaveLayer(const Layer<VoxelType>& layer, const std::string& file_path) {
  return layer.saveToFile(file_path);
}

template <typename VoxelType>
bool SaveLayerSubset(const Layer<VoxelType>& layer,
                     const std::string& file_path,
                     BlockIndexList blocks_to_include,
                     bool include_all_blocks) {
  return layer.saveSubsetToFile(file_path, blocks_to_include,
                                include_all_blocks);
}

}  // namespace io

}  // namespace voxblox

#endif  // VOXBLOX_CORE_IO_LAYER_IO_H_
