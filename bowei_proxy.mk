##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=bowei_proxy
ConfigurationName      :=Debug
WorkspacePath          := "/home/boweiliu/Documents/codelite_workspace"
ProjectPath            := "/home/boweiliu/Documents/codelite_workspace/bowei_proxy"
IntermediateDirectory  :=./Debug
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=Bowei-Liu
Date                   :=31/10/15
CodeLitePath           :="/home/boweiliu/.codelite"
LinkerName             :=/usr/bin/g++
SharedObjectLinkerName :=/usr/bin/g++ -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="bowei_proxy.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). 
IncludePCH             := 
RcIncludePath          := 
Libs                   := 
ArLibs                 :=  
LibPath                := $(LibraryPathSwitch). 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := /usr/bin/ar rcu
CXX      := /usr/bin/g++
CC       := /usr/bin/gcc
CXXFLAGS :=  -g -O0 -Wall $(Preprocessors)
CFLAGS   :=  -g -O0 -Wall $(Preprocessors)
ASFLAGS  := 
AS       := /usr/bin/as


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IntermediateDirectory)/Utility.cpp$(ObjectSuffix) $(IntermediateDirectory)/CachedDocManager.cpp$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

MakeIntermediateDirs:
	@test -d ./Debug || $(MakeDirCommand) ./Debug


$(IntermediateDirectory)/.d:
	@test -d ./Debug || $(MakeDirCommand) ./Debug

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/main.cpp$(ObjectSuffix): main.cpp $(IntermediateDirectory)/main.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/boweiliu/Documents/codelite_workspace/bowei_proxy/main.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/main.cpp$(DependSuffix): main.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/main.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/main.cpp$(DependSuffix) -MM "main.cpp"

$(IntermediateDirectory)/main.cpp$(PreprocessSuffix): main.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/main.cpp$(PreprocessSuffix) "main.cpp"

$(IntermediateDirectory)/Utility.cpp$(ObjectSuffix): Utility.cpp $(IntermediateDirectory)/Utility.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/boweiliu/Documents/codelite_workspace/bowei_proxy/Utility.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Utility.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Utility.cpp$(DependSuffix): Utility.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/Utility.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/Utility.cpp$(DependSuffix) -MM "Utility.cpp"

$(IntermediateDirectory)/Utility.cpp$(PreprocessSuffix): Utility.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Utility.cpp$(PreprocessSuffix) "Utility.cpp"

$(IntermediateDirectory)/CachedDocManager.cpp$(ObjectSuffix): CachedDocManager.cpp $(IntermediateDirectory)/CachedDocManager.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "/home/boweiliu/Documents/codelite_workspace/bowei_proxy/CachedDocManager.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/CachedDocManager.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/CachedDocManager.cpp$(DependSuffix): CachedDocManager.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/CachedDocManager.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/CachedDocManager.cpp$(DependSuffix) -MM "CachedDocManager.cpp"

$(IntermediateDirectory)/CachedDocManager.cpp$(PreprocessSuffix): CachedDocManager.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/CachedDocManager.cpp$(PreprocessSuffix) "CachedDocManager.cpp"


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Debug/


