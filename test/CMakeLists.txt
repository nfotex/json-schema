LINK_DIRECTORIES(${JSONCPP_LIBRARY_DIRS})

SET(TEST_PROGRAMS
  test-uri
  test-validate
  )

SET(ENV{srcdir} ${CMAKE_CURRENT_SOURCE_DIR})

FOREACH(PROGRAM ${TEST_PROGRAMS})
  ADD_EXECUTABLE(${PROGRAM} ${PROGRAM}.cc)
  TARGET_LINK_LIBRARIES(${PROGRAM} json-schema)
ENDFOREACH()

TARGET_LINK_LIBRARIES(test-validate ${JSONCPP_LIBRARIES})

ADD_CUSTOM_TARGET(cleanup
  COMMAND ${CMAKE_COMMAND} -DDIR=${CMAKE_CURRENT_BINARY_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/cleanup.cmake
  )

SET(EXTRA_TESTS
  p-option/t001.test
  p-option/t002.test
  p-option/t003.test
  p-option/t010.test
  p-option/t011.test
  p-option/t020.test
  p-option/t021.test
  p-option/t022.test
  p-option/t030.test
  p-option/t031.test
  p-option/t050.test
  p-option/t051.test
  p-option/t052.test
  p-option/t060.test
  p-option/t061.test
  p-option/t070.test
  p-option/t071.test
  )

FOREACH(CASE ${EXTRA_TESTS})
  ADD_TEST(${CASE} perl ${CMAKE_BINARY_DIR}/test/runtest ${CMAKE_CURRENT_SOURCE_DIR}/${CASE})
  SET_TESTS_PROPERTIES(${CASE} PROPERTIES SKIP_RETURN_CODE 77)
ENDFOREACH()

SET(DRAFT7_TESTS
  additionalItems.json
  additionalProperties.json
  allOf.json
  anyOf.json
  boolean_schema.json
  const.json
  contains.json
  default.json
  definitions.json
  dependencies.json
  enum.json
  exclusiveMaximum.json
  exclusiveMinimum.json
  if-then-else.json
  items.json
  maxItems.json
  maxLength.json
  maxProperties.json
  maximum.json
  minItems.json
  minLength.json
  minProperties.json
  minimum.json
  multipleOf.json
  not.json
  oneOf.json
  pattern.json
  patternProperties.json
  properties.json
  propertyNames.json
  ref.json
  required.json
  type.json
  uniqueItems.json
  )
# this test feature is not implemented
#        refRemote.json

FOREACH(CASE ${DRAFT7_TESTS})
  ADD_TEST(${CASE} ${CMAKE_BINARY_DIR}/test/test-validate ${CMAKE_CURRENT_SOURCE_DIR}/draft7/${CASE})
ENDFOREACH()

INCLUDE_DIRECTORIES(${JSONCPP_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/..)

ADD_CUSTOM_TARGET(check COMMAND ${CMAKE_CTEST_COMMAND})
