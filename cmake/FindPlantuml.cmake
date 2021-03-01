# Finds the path to the plantuml.jar file
#
# Adds the following variables
#
#   PLANTUML_JAR_PATH

find_path(PlantumlJar_DIR
    NAMES plantuml.jar
    PATHS "/usr/share/java/plantuml/" "/usr/share/plantuml/"
    NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Plantuml
    REQUIRED_VARS PlantumlJar_DIR)

set(PLANTUML_JAR_PATH ${PlantumlJar_DIR}/plantuml.jar)


