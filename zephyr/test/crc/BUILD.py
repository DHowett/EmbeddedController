# Copyright 2021 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Register zmake project for crc test."""

register_host_test("crc", dts_overlays=[here / "boards/native_posix.overlay"])