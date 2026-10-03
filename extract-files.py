#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: 2024 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

from extract_utils.fixups_blob import blob_fixup, blob_fixups_user_type
from extract_utils.fixups_lib import (
    lib_fixup_remove,
    lib_fixups,
    lib_fixups_user_type,
)
from extract_utils.main import ExtractUtils, ExtractUtilsModule

namespace_imports = [
    "device/nothing/phone2",
    "vendor/qcom/common/vendor/display",
    "vendor/qcom/common/vendor/perf",
    "vendor/qcom/common/vendor/qseecomd",
    "vendor/qcom/common/vendor/wlan",
]


def lib_fixup_vendor_suffix(lib: str, partition: str, *args, **kwargs):
    return f'{lib}_{partition}' if partition == 'vendor' else None


def lib_fixup_prebuilt_suffix(lib: str, *args, **kwargs):
    return f'{lib}_prebuilt'


lib_fixups: lib_fixups_user_type = {
    **lib_fixups,
    'libgrpc++_unsecure': lib_fixup_prebuilt_suffix,
    'libloc_api_v02': lib_fixup_prebuilt_suffix,
    (
        'com.qualcomm.qti.dpm.api@1.0',
        'com.qualcomm.qti.imscmservice@*',
        'com.qualcomm.qti.uceservice@*',
        'libmmosal',
        'vendor.qti.ImsRtpService-V1-ndk',
        'vendor.qti.data.factory@*',
        'vendor.qti.data.factoryservice-V1-ndk',
        'vendor.qti.data.mwqem@1.0',
        'vendor.qti.data.mwqemaidlservice-V1-ndk',
        'vendor.qti.data.slm@1.0',
        'vendor.qti.diaghal@1.0',
        'vendor.qti.hardware.ListenSoundModel@1.0',
        'vendor.qti.hardware.data.cne.internal.*@*',
        'vendor.qti.hardware.data.cneaidlservice.internal.api-V1-ndk',
        'vendor.qti.hardware.data.cneaidlservice.internal.constants-V1-ndk',
        'vendor.qti.hardware.data.cneaidlservice.internal.server-V1-ndk',
        'vendor.qti.hardware.data.connection@*',
        'vendor.qti.hardware.data.connectionfactory-V1-ndk',
        'vendor.qti.hardware.data.connectionfactory-V1-ndk_platform',
        'vendor.qti.hardware.data.dataactivity-V1-ndk',
        'vendor.qti.hardware.data.dataactivity-V1-ndk_platform',
        'vendor.qti.hardware.data.dynamicdds@*',
        'vendor.qti.hardware.data.dynamicddsaidlservice-V1-ndk',
        'vendor.qti.hardware.data.flow@1.0',
        'vendor.qti.hardware.data.flow@1.1',
        'vendor.qti.hardware.data.flowaidlservice-V1-ndk',
        'vendor.qti.hardware.data.iwlan@*',
        'vendor.qti.hardware.data.ka-V1-ndk',
        'vendor.qti.hardware.data.ka-V1-ndk_platform',
        'vendor.qti.hardware.data.latency@1.0',
        'vendor.qti.hardware.data.lce@1.0',
        'vendor.qti.hardware.data.lceaidlservice-V1-ndk',
        'vendor.qti.hardware.data.qmi@1.0',
        'vendor.qti.hardware.data.qmiaidlservice-V1-ndk',
        'vendor.qti.hardware.dpmaidlservice-V1-ndk',
        'vendor.qti.hardware.dpmservice@*',
        'vendor.qti.hardware.embmssl@*',
        'vendor.qti.hardware.embmsslaidl-V2-ndk',
        'vendor.qti.hardware.limits@*',
        'vendor.qti.hardware.mwqemadapter@1.0',
        'vendor.qti.hardware.mwqemadapteraidlservice-V1-ndk',
        'vendor.qti.hardware.qccsyshal@*',
        'vendor.qti.hardware.qccvndhal@1.0',
        'vendor.qti.hardware.radio.am@1.0',
        'vendor.qti.hardware.radio.atcmdfwd@1.0',
        'vendor.qti.hardware.radio.ims-V12-ndk_platform',
        'vendor.qti.hardware.radio.ims@*',
        'vendor.qti.hardware.radio.internal.deviceinfo@1.0',
        'vendor.qti.hardware.radio.lpa@*',
        'vendor.qti.hardware.radio.qcrilhook@1.0',
        'vendor.qti.hardware.radio.qtiradio-V8-ndk_platform',
        'vendor.qti.hardware.radio.qtiradio@*',
        'vendor.qti.hardware.radio.qtiradioconfig-V2-ndk_platform',
        'vendor.qti.hardware.radio.uim@*',
        'vendor.qti.hardware.radio.uim_remote_client@*',
        'vendor.qti.hardware.radio.uim_remote_server@1.0',
        'vendor.qti.hardware.slmadapter@1.0',
        'vendor.qti.hardware.wifidisplaysession@1.0',
        'vendor.qti.ims.callcapability@1.0',
        'vendor.qti.ims.callcapabilityaidlservice-V1-ndk',
        'vendor.qti.ims.callinfo@1.0',
        'vendor.qti.ims.configaidlservice-V1-ndk',
        'vendor.qti.ims.configservice@*',
        'vendor.qti.ims.connection@1.0',
        'vendor.qti.ims.connectionaidlservice-V1-ndk',
        'vendor.qti.ims.factory@*',
        'vendor.qti.ims.factoryaidlservice-V1-ndk',
        'vendor.qti.ims.imscmaidlservice-V1-ndk',
        'vendor.qti.ims.rcsconfig@*',
        'vendor.qti.ims.rcssip@*',
        'vendor.qti.ims.rcssipaidlservice-V1-ndk',
        'vendor.qti.ims.rcsuce@*',
        'vendor.qti.ims.rcsuceaidlservice-V1-ndk',
        'vendor.qti.ims.uceaidlservice-V1-ndk',
        'vendor.qti.diaghal-V1-ndk',
        'vendor.qti.hardware.cacertaidlservice-V1-ndk',
        'vendor.qti.hardware.minkipcbinder-V1-ndk',
        'vendor.qti.qccsyshal_aidl-V1-ndk',
        'vendor.qti.qccvndhal_aidl-V1-ndk',
        'vendor.qti.imsrtpservice@3.0',
        'vendor.qti.latency@*',
        'vendor.display.color@1.0',
        'vendor.display.color@1.1',
        'vendor.display.color@1.2',
        'vendor.display.color@1.3',
        'vendor.display.color@1.4',
        'vendor.display.color@1.5',
        'vendor.display.postproc@1.0',
        'vendor.qti.hardware.iop@2.0',
        'vendor.qti.hardware.perf@2.0',
        'vendor.qti.hardware.perf@2.1',
        'vendor.qti.hardware.perf@2.2',
        'vendor.qti.hardware.perf2-V1-ndk',
        'vendor.qti.latencyaidlservice-V1-ndk',
        'vendor.qti.qspmhal-V1-ndk',
    ): lib_fixup_vendor_suffix,
    (
        'libloc_socket',
        'libsynergy_loc_api',
        'libthermalclient',
        'vendor.qti.memory.pasrmanager-V1-ndk',
        'vendor.qti.snapdragonServices-V2-ndk',
        'vendor.qti.snapdragonServices.qape-V1-ndk',
    ): lib_fixup_remove,
}

blob_fixups: blob_fixups_user_type = {
    (
        'vendor/bin/sensors.qti',
        'vendor/lib64/libsns_direct_channel_stub.so',
        'vendor/lib64/libsnsdiaglog.so',
        'vendor/lib64/libssc.so',
    ): blob_fixup()
        .replace_needed('libprotobuf-cpp-lite-21.7.so', 'libprotobuf-cpp-lite-21.12.so')
        .replace_needed('libsnsapi.so', 'libsnsapi_v2.so'),
    (
        'vendor/lib64/libsns_direct_channel_stub-pong.so',
        'vendor/lib64/libsnsdiaglog-pong.so',
        'vendor/lib64/libssc-pong.so',
        'vendor/lib64/sensors.ssc.so',
    ): blob_fixup()
        .fix_soname()
        .replace_needed('libsns_direct_channel_stub.so', 'libsns_direct_channel_stub-pong.so')
        .replace_needed('libsnsdiaglog.so', 'libsnsdiaglog-pong.so')
        .replace_needed('libssc.so', 'libssc-pong.so'),
    'vendor/lib64/libsnsapi_v2.so': blob_fixup()
        .replace_needed('libprotobuf-cpp-lite-21.7.so', 'libprotobuf-cpp-lite-21.12.so')
        .fix_soname(),
    'vendor/bin/slim_daemon': blob_fixup()
        .replace_needed('libsnsapi.so', 'libsnsapi_v2.so'),
    'system_ext/etc/seccomp_policy/perfservice.policy': blob_fixup()
        .add_line_if_missing('lseek: 1'),
    'vendor/etc/seccomp_policy/qcrilnr@2.0.policy': blob_fixup()
        .add_line_if_missing('getppid: 1')
        .add_line_if_missing('uname: 1'),
    'vendor/etc/audio/sku_cape/resourcemanager_waipio_qrd.xml': blob_fixup()
        .regex_replace('<speaker_protection_enabled>1<', '<speaker_protection_enabled>0<'),
    ('vendor/bin/hw/android.hardware.security.keymint-service-qti', 'vendor/lib64/libqtikeymint.so'): blob_fixup()
        .replace_needed('android.hardware.security.keymint-V1-ndk_platform.so', 'android.hardware.security.keymint-V1-ndk.so')
        .replace_needed('android.hardware.security.secureclock-V1-ndk_platform.so', 'android.hardware.security.secureclock-V1-ndk.so')
        .replace_needed('android.hardware.security.sharedsecret-V1-ndk_platform.so', 'android.hardware.security.sharedsecret-V1-ndk.so')
        .add_needed('android.hardware.security.rkp-V1-ndk.so'),
    ('vendor/bin/hw/android.hardware.identity-service-qti', 'vendor/lib64/libqtiidentitycredential.so'): blob_fixup()
        .replace_needed('android.hardware.identity-V3-ndk_platform.so', 'android.hardware.identity-V3-ndk.so')
        .replace_needed('android.hardware.keymaster-V3-ndk_platform.so', 'android.hardware.keymaster-V3-ndk.so'),
    ('vendor/bin/hw/vendor.qti.hardware.vibrator.service', 'vendor/lib64/vendor.qti.hardware.vibrator.impl.so'): blob_fixup()
        .replace_needed('android.hardware.vibrator-V2-ndk_platform.so', 'android.hardware.vibrator-V2-ndk.so'),
    ('vendor/etc/media_codecs.xml', 'vendor/etc/media_codecs_cape.xml', 'vendor/etc/media_codecs_cape_vendor.xml'): blob_fixup()
        .regex_replace('.*media_codecs_(google_audio|google_c2|google_telephony|vendor_audio).*\n', ''),
    'vendor/lib64/libcamximageformatutils.so': blob_fixup()
        .replace_needed('android.hardware.graphics.allocator-V1-ndk.so', 'android.hardware.graphics.allocator-V2-ndk.so'),
    ('vendor/lib64/libgarden.so', 'vendor/lib64/libgarden_haltests_e2e.so'): blob_fixup()
        .replace_needed('android.hardware.gnss-V1-ndk_platform.so', 'android.hardware.gnss-V1-ndk.so'),
    'vendor/lib64/libmorpho_video_stabilizer.so': blob_fixup()
        .add_needed('libutils.so'),
    'vendor/lib64/libwvhidl.so': blob_fixup()
        .add_needed('libcrypto_shim.so'),
    'vendor/lib64/nfc_nci_nxp_snxxx.so': blob_fixup()
        .add_needed('libbase_shim.so'),
    'vendor/lib64/vendor.libdpmframework.so': blob_fixup()
        .add_needed('libhidlbase_shim.so'),
    'vendor/lib64/libntcamallocator.so': blob_fixup()
        .add_needed('libui_shim.so'),
    'vendor/lib64/vendor.noth.hardware.camera-service-impl.so': blob_fixup()
        .add_needed('libui_shim.so')
        .add_needed('libntcam_shim.so')
        .binary_regex_replace(
            b'_ZN7android19GraphicBufferMapper12importBufferEPK13native_handlejjjimjPS3_',
            b'_ZN7android19GraphicBufferMapper12importBuffeREPK13native_handlejjjimjPS3_',
        ),
    'vendor/bin/thermal-engine-v2': blob_fixup()
        .binary_regex_replace(
            b'/topology/physical_package_id',
            b'/topology/cluster_id\x00\x00\x00\x00\x00\x00\x00\x00\x00',
        ),
    'vendor/etc/init/vendor.noth.hardware.camera-service.rc': (
        blob_fixup().regex_replace(r'\bNtCamAlgoCapacity\b', 'CameraServiceCapacity')
    ),
    'vendor/lib64/libntcamextened.so': blob_fixup()
        .replace_needed('libntcamera2ndk_vendor_v1.so', 'libntcamera2ndk_vendor_v3.so')
        .add_needed('libcamhidl_shim.so')
        .binary_regex_replace(b'ACameraManager_getCameraIdList', b'ACameraManager_getCameraIdShim'),
    'vendor/lib64/libntofflinepostproc.so': blob_fixup()
        .replace_needed('vendor.qti.hardware.camera.postproc@1.0.so', 'vendor.qti.hardware.camera.postproc@1.0-nothing.so')
        .add_needed('libcamhidl_shim.so')
        .binary_regex_replace(b'16IPostProcService10getService', b'16IPostProcService10getShimSvc'),
    'vendor/lib64/vendor.qti.hardware.camera.postproc@1.0-service-impl.so': blob_fixup()
        .replace_needed('vendor.qti.hardware.camera.postproc@1.0.so', 'vendor.qti.hardware.camera.postproc@1.0-nothing.so')
        .add_needed('libcamhidl_shim.so')
        .binary_regex_replace(b'16IPostProcService17registerAsService', b'16IPostProcService17registerAsShimSvc')
        .binary_regex_replace(b'7IMapper10getService', b'7IMapper10getShimSvc'),
    'vendor/lib64/vendor.qti.hardware.camera.aon@1.0-service-impl.so': blob_fixup()
        .add_needed('libcamhidl_shim.so')
        .binary_regex_replace(b'11IAONService17registerAsService', b'11IAONService17registerAsShimSvc'),
}  # fmt: skip

module = ExtractUtilsModule(
    'phone2',
    'nothing',
    blob_fixups=blob_fixups,
    lib_fixups=lib_fixups,
    namespace_imports=namespace_imports,
)

if __name__ == '__main__':
    utils = ExtractUtils.device(module)
    utils.run()
