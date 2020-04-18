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

#include <assert.h> 
#include <iostream>
#include <string.h>
#include <netinet/in.h> // htons()

#include "connman-client.h"
#include "information-element.h"

int main (int argc, const char **argv)
{
    // check that packing works
    if (sizeof(P2P::DeviceInformationSubelement) !=
        P2P::SubelementSize[P2P::DEVICE_INFORMATION] ||
        sizeof(P2P::AssociatedBSSIDSubelement) !=
        P2P::SubelementSize[P2P::ASSOCIATED_BSSID] ||
        sizeof(P2P::CoupledSinkInformationSubelement) !=
        P2P::SubelementSize[P2P::COUPLED_SINK_INFORMATION]) {
        std::cout << "Subelement struct size checks failed"<< std::endl;
        return 1;
    }

    P2P::InformationElement ie;
    auto sub_element = P2P::new_subelement(P2P::DEVICE_INFORMATION);
    auto dev_info = (P2P::DeviceInformationSubelement*)sub_element;
    dev_info->session_management_control_port =  htons(8080);
    dev_info->maximum_throughput = htons(50);
    dev_info->field1.device_type = P2P::PRIMARY_SINK;
    dev_info->field1.session_availability = true;

    ie.add_subelement (sub_element);
    ie.add_subelement (P2P::new_subelement(P2P::COUPLED_SINK_INFORMATION));
    ie.add_subelement (P2P::new_subelement(P2P::ASSOCIATED_BSSID));

    auto array = ie.serialize ();
    P2P::InformationElement ie2(array);
    auto array1 = ie.to_string();
    auto array2 = ie2.to_string();
    if (array1 != array2) {
        std::cout << "Expected byte array '" << array1
                  << "', got '" << array2 << "'" << std::endl;
        return 1;
    }

    
    return 0;
}
