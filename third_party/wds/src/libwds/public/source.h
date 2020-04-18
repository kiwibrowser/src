/*
 * This file is part of Wireless Display Software for Linux OS
 *
 * Copyright (C) 2014 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef LIBWDS_PUBLIC_SOURCE_H_
#define LIBWDS_PUBLIC_SOURCE_H_

#include "peer.h"

namespace wds {

class SourceMediaManager;

/**
 * Source state machine implementation
 */
class WDS_EXPORT Source : public Peer {
 public:
  /**
   * Factory method that creates Source state machine.
   * @param delegate that is used for networking
   * @param media manger that is used for media stream management
   * @param observer
   * @return newly created Source instance
   */
  static Source* Create(Peer::Delegate* delegate,
                        SourceMediaManager* mng,
                        Peer::Observer* observer = nullptr);
};

}

#endif // LIBWDS_PUBLIC_SOURCE_H_
