//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) 2015 Guillaume Blanc                                         //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#ifndef OZZ_OZZ_ANIMATION_RUNTIME_SKELETON_H_
#define OZZ_OZZ_ANIMATION_RUNTIME_SKELETON_H_

#include "ozz/base/platform.h"
#include "ozz/base/io/archive_traits.h"

namespace ozz {
namespace io { class IArchive; class OArchive; }
namespace math { struct SoaTransform; }
namespace animation {

// Forward declaration of SkeletonBuilder, used to instantiate a skeleton.
namespace offline { class SkeletonBuilder; }

// This runtime skeleton data structure provides a const-only access to joint
// hierarchy, joint names and bind-pose. This structure is filled by the
// SkeletonBuilder and can be serialize/deserialized.
// Joint names, bind-poses and hierarchy information are all stored in separate
// arrays of data (as opposed to joint structures for the RawSkeleton), in order
// to closely match with the way runtime algorithms use them. Joint hierarchy is
// packed as an array of 16 bits element (JointProperties) per joint, stored in
// breadth-first order. JointProperties::parent member is enough to traverse the
// whole joint hierarchy in breadth-first order. JointProperties::is_leaf is a
// helper that is used to speed-up some algorithms: See IterateJointsDF() from
// skeleton_utils.h that implements a depth-first traversal utility.
class Skeleton {
 public:

  // Defines Skeleton constant values.
  enum Constants {
    // Limits the number of joints in order to control the number of bits
    // required to store a joint index. Limiting the number of joints also helps
    // handling worst size cases, like when it is required to allocate an array
    // of joints on the stack.
    // The definitive value isn't decided yet, it'll depend on the number of
    // bits available in the runtime animation key frame structure once it'll be
    // compressed.
    kMaxJointsNumBits = 10,

    // Defines the maximum number of joints.
    // Reserves one index (the last) for kNoParentIndex value.
    kMaxJoints = (1<<kMaxJointsNumBits) - 1,

    // Defines the maximum number of SoA elements required to store the maximum
    // number of joints.
    kMaxSoAJoints = (kMaxJoints + 3) / 4,

    // Defines the index of the parent of the root joint (which has no parent in
    // fact).
    kNoParentIndex = kMaxJoints,
  };

  // Builds a default skeleton.
  Skeleton();

  // Declares the public non-virtual destructor.
  ~Skeleton();

  // Returns the number of joints of *this skeleton.
  int num_joints() const {
    return num_joints_;
  }

  // Returns the number of soa elements matching the number of joints of *this
  // skeleton. This value is useful to allocate SoA runtime data structures.
  int num_soa_joints() const {
      return (num_joints_ + 3) / 4;
  }

  // Per joint properties.
  struct JointProperties {
    // Parent's index, kNoParentIndex for the root.
    uint16_t parent: Skeleton::kMaxJointsNumBits;
    
    // Set to 1 for a leaf, 0 for a branch.
    uint16_t is_leaf: 1;
  };

  // Returns joint's parent indices range.
  Range<const JointProperties> joint_properties() const {
    return Range<const JointProperties>(joint_properties_, num_joints_);
  }

  // Returns joint's bind poses. Bind poses are stored in soa format.
  Range<const math::SoaTransform> bind_pose() const;

  // Returns joint's name collection.
  const char* const* joint_names() const {
    return joint_names_;
  }

  // Serialization functions.
  // Should not be called directly but through io::Archive << and >> operators.
  void Save(ozz::io::OArchive& _archive) const;
  void Load(ozz::io::IArchive& _archive, uint32_t _version);

 private:

  // Disables copy and assignation.
  Skeleton(Skeleton const&);
  void operator=(Skeleton const&);

  // Internal destruction function.
  void Destroy();

  // SkeletonBuilder class is allowed to instantiate an Skeleton.
  friend class offline::SkeletonBuilder;

  // Buffers below store joint informations in DAG order. Their size is equal to
  // the number of joints of the skeleton.

  // Array of joint properties.
  JointProperties* joint_properties_;

  // Bind pose of every joint in local space.
  math::SoaTransform* bind_pose_;

  // Stores the name of every joint in an array of c-strings.
  // Uses a single allocation to store the array and all the c strings.
  char** joint_names_;

  // The number of joints.
  int num_joints_;
};
}  // animation

namespace io {
OZZ_IO_TYPE_VERSION(1, animation::Skeleton)
OZZ_IO_TYPE_TAG("ozz-skeleton", animation::Skeleton)
}  // io
}  // ozz
#endif  // OZZ_OZZ_ANIMATION_RUNTIME_SKELETON_H_
