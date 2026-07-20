# Header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO powerof3/CLibUtil
    REF 5560aae4ae6db4f28b8d55ae3b3d651b1adc18e4
    SHA512 26bffc8c6d74749cb9dca86003c2d7fb99c6a2f6f6bde663dcaf9e0808a886f1b7ce0497e5359244dceb9bf0cd4b5a478e67b009e3581a970c815f5c0c74bcf7
    HEAD_REF master
)

# Install codes
set(CLIBUTIL_SOURCE	${SOURCE_PATH}/include/ClibUtil)
file(INSTALL ${CLIBUTIL_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
