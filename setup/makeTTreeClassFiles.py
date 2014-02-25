#!/usr/bin/python

# ---------------------------------------------------------------------------------- #
#  
#  	Python macro to make C++ wrapper to the ROOT TTree to faciliate basic
#  	branch excessing (e.g. CMS2.h/cc). For example if a branch has a branch
#  	named "track_p4", a function wrapper called track_p4() will be produced to
#  	access the data in this branch via a looper. This is an extention to the
#  	"makeCMS2ClassFiles.C" and will work on CMS2 ntuples and simple baby ntuples.
#  	Also, if the TTree was produced by CMSSW, an option can be toggled to produce
#  	code that can be compiled in CMSSW (and thus use an EDAnalyzer, etc.).
#   
#   Usage:
#   ./makeTTreeClassFiles.py --file_name=<filename>
#   
#   Required:
#   --file_name : The name of the ROOT file with the TTree (required)
#   
#   Options:
#   --tree_name : The name of the TTree (default: "Events")
#   --namespace : The namespace to use (default: "tas")
#   --obj_name  : The object name to use (default: "cms2")
#   --class_name: The class name to use (default: "CMS2")
#   
#   CMSSW Options (off by default)
#   --use_scram : Toggle to support scram (default: false)
#   --use_cmssw : Toggle to support CMSSW (default: false)
#   --no_trig   : Toggle to not include the trigger functions (default: false)
#   
#   Example CMS2.h with CMSSW:
#    ./makeTTreeClassFiles.py --file_name=cms2_ntuple_postprocessed.root --use_cmssw --no_trig
#   
#   Example for baby: 
#   ./makeTTreeClassFiles.py --file_name=baby.root --no_trig --tree_name tree --namespace ssb --class_name SSB2012 --obj_name samesignbtag
#  
# ---------------------------------------------------------------------------------- #


import os
import sys
from optparse import OptionParser

# parse inputs
# ---------------------------------------------------------------------------------- #

parser = OptionParser()

# parameter options
parser.add_option("--file_name" , dest="file_name" , default=""      , help="The name of the ROOT file with the TTree (required)")
parser.add_option("--tree_name" , dest="tree_name" , default="Events", help="The name of the TTree (default: \"Events\")"        )
parser.add_option("--namespace" , dest="namespace" , default="tas"   , help="The namespace to use (default: \"tas\")"            )
parser.add_option("--obj_name"  , dest="obj_name"  , default="cms2"  , help="The object name to use (default: \"cms2\")"         )
parser.add_option("--class_name", dest="class_name", default="CMS2"  , help="The class name to use (default: \"CMS2\")"          )

# boolean options
parser.add_option("--use_scram", action="store_true", dest="use_scram", default=False , help="Toggle to support scram (default: \"false\")"                    )
parser.add_option("--use_cmssw", action="store_true", dest="use_cmssw", default=False , help="Toggle to support CMSSW (default: \"false\")"                    )
parser.add_option("--no_trig"  , action="store_true", dest="no_trig"  , default=False , help="Toggle to not include the trigger functions (default: \"false\")")

(options, args) = parser.parse_args()


# check for validity
# ---------------------------------------------------------------------------------- #

def CheckOptions():
	# file_name
	if (not options.file_name):
		raise Exception("required file_name is missing")

	if (not os.path.isfile(options.file_name)):
		raise Exception("required file_name does not exist")
 
	# tree_name
	if (not options.tree_name):
		raise Exception("tree_name is blank")

	# namespace
	if (not options.namespace):
		raise Exception("namespace is blank")

	# obj_name
	if (not options.obj_name):
		raise Exception("obj_name is blank")

	# class_name
	if (not options.class_name):
		raise Exception("class_name is blank")


# Branch Info 
# ---------------------------------------------------------------------------------- #

from ROOT import gROOT, TBranch, TFile, TTree, TList

class BranchInfo:
	def __init__(self, alias, branch):
		self.branch = branch
		self.alias  = alias
		return

	def __str__(self):
		return "branch: name = %s, alias = %s, class = %s" % (self.GetBranchName(), self.GetAliasName(), self.GetClassType())

	def Alias(self):
		return self.alias 

	def Branch(self):
		return self.branch 

	def GetBranchName(self):
		return self.branch.GetName() 

	def GetAliasName(self):
		if self.alias:
			return self.alias.GetName() 
		else:
			return None
	
	def IsEdmBranch(self):
		return ("edm::Wrapper" in self.branch.GetClassName())

	def GetClassType(self):
		class_type = self.branch.GetClassName()

		# add std:: to vector types
		class_type = class_type.replace("vector", "std::vector")

		# replace LorentzVectors
		class_type = class_type.replace("ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<float> >" , "LorentzVector")
		class_type = class_type.replace("ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> >", "LorentzVectorD")

		# test if its a edm::Wapper type
		# TODO: do this better --> REGEX?
		if "edm::Wrapper" in class_type:
			class_type = class_type.replace("edm::Wrapper<", "")
			class_type = class_type.replace(">", "", 1)
			class_type = class_type.replace(" >", ">")
			class_type = class_type.replace(">>>>", "> > > >")
			class_type = class_type.replace(">>>", "> > >")
			class_type = class_type.replace(">>", "> >")

		# test if its a build in type
		elif not class_type:
			class_type = self.branch.GetTitle()
			type_char  = class_type.rsplit("/")[1]
			builtin_types = {
            		"B": "char",
            		"b": "unsigned char",
            		"S": "short",
            		"s": "unsigned short",
            		"I": "int",
            		"i": "unsigned int",
            		"F": "float",
            		"D": "double",
            		"L": "long",
            		"l": "unsigned long",
            		"O": "bool",
				}
			class_type = builtin_types[type_char]

		return class_type 

	def GetLabel(self):
		return self.GetBranchName().split("_")[1]

	def GetInstance(self):
		return self.GetBranchName().split("_")[2]
	
	def GetAccessorName(self):
		if self.alias:
			acc_str = self.GetAliasName()
		else:
			acc_str = self.GetBranchName()
		return acc_str

	def GetHandleName(self):
		return "%s_handle" % self.GetAccessorName() 

	def GetHandleDeclaration(self, namespace, use_cmssw = False):
 		if use_cmssw:
			return "%s::EdmHandleWrapper< %s > %s;\n" % (namespace, self.GetClassType(), self.GetHandleName());
		else:
			return "%s::Handle< %s > %s;\n" % (namespace, self.GetClassType(), self.GetHandleName());

	def GetInitializer(self, use_cmssw = False):
		if use_cmssw:
			return "%s(\"%s\", \"%s\")" % (self.GetHandleName(), self.GetLabel(), self.GetInstance())
		else:
			return "%s(\"%s\")" % (self.GetHandleName(), self.GetBranchName())

	def GetInitCall(self):
		return "%s.Init(tree);\n" % (self.GetHandleName())

	def GetClearCall(self):
		return "%s.Clear();\n" % (self.GetHandleName())

	def GetEntryCall(self):
		return "%s.GetEntry(entry);\n" % (self.GetHandleName())

	def GetLoadAllBranchesCall(self):
		return "%s.Load();\n" % (self.GetHandleName())

	def SetEventCall(self):
		return "%s.SetEvent(event);\n" % (self.GetHandleName())
	
	def GetAccessorDefinition(self, class_name):
		return "const %s& %s::%s() {return %s.get();}\n" % (self.GetClassType(), class_name, self.GetAccessorName(), self.GetHandleName())

	def GetAccessorWrapper(self, obj_name):
		return "const %s& %s() {return %s.%s();}\n" % (self.GetClassType(), self.GetAccessorName(), obj_name, self.GetAccessorName())

	def GetAccessorDeclaration(self):
		return "const %s& %s();\n" % (self.GetClassType(), self.GetAccessorName())

# header 
# ---------------------------------------------------------------------------------- #

def HeaderString(branch_infos, use_cmssw):
	if use_cmssw:
		header_str = """#ifndef CLASSNAME_H
#define CLASSNAME_H

#include "Math/LorentzVector.h"
#include "Math/Point3D.h"
#include "TMath.h"
#include "TBranch.h"
#include "TTree.h"
#include "TH1F.h"
#include "TFile.h"
#include "TBits.h"
#include "TString.h"
#include <vector> 
#include <string>          

typedef ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<float> > LorentzVector;
typedef ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> > LorentzVectorD;

// ################################################################################################# //
// CMSSW independent
// ################################################################################################# //
#ifndef CMS2_USE_CMSSW

// Handle Classes to handle branch loading 
// ------------------------------------------------------------------------------------------------- //

namespace NAMESPACE
{
    struct BranchType
    {
        enum value_type
        {
            CLASS,
            BUILTIN,
            NOT_SET,

            static_size
        };
    };

    template <typename T>
    class Handle
    {
        public:
            
            // construct: 
            explicit Handle(const std::string& branch_name);

            // destroy:
            virtual ~Handle() {}

            // set the branch's entry 
            void GetEntry(const unsigned int entry);

            // initialize the handle's branches
            void Init(TTree& tree);

            // is the branch already loaded
            bool IsLoaded() const;

            // load the branch
            void Load();

            // clear the branch
            void Clear();

            // get the value
            const T& get();

        protected:

            // set the branch type private member (based on the TBranch) 
            void SetBranchType(const std::string& branch_class);

            // members:
            bool                   m_is_loaded;
            unsigned int           m_entry;
            std::string            m_branch_name;
            T*                     m_object_ptr;
            T                      m_object;
            BranchType::value_type m_branch_type;

            // the TTree owns the branch (this is only a reference pointer)
            TBranch*     m_branch;

        private:
            // no default constructor
            Handle() {}
    };

    template <typename T>
    /*explicit*/ Handle<T>::Handle(const std::string& branch_name)
        : m_is_loaded(false)
        , m_entry(0)
        , m_branch_name(branch_name)
        , m_object_ptr(NULL)
        , m_object()
        , m_branch_type(BranchType::NOT_SET)
        , m_branch(NULL)
    {
    }

    template <typename T>
    void Handle<T>::GetEntry(const unsigned int entry)
    {
        Clear();
        m_entry = entry;
    }

    template <typename T>
    bool Handle<T>::IsLoaded() const
    {
        return m_is_loaded;
    }
   
    template <typename T>
    void Handle<T>::Clear()
    {
        m_is_loaded = false;
    }

    template <typename T>
    void Handle<T>::Load()
    {
        if (m_branch)
        {
            m_branch->GetEntry(m_entry);
            m_is_loaded = true;
        }
        else
        {
            throw std::runtime_error("[CLASSNAME] ERROR: branch " + m_branch_name + " does not exist!"); 
        }
    }

    template <typename T>
    void Handle<T>::SetBranchType(const std::string& branch_class)
    {
        if (branch_class.find("edm::Wrapper")!=std::string::npos)
        {
            m_branch_type = BranchType::BUILTIN;
        }
        else if 
        (
            branch_class.find("/B")!=std::string::npos || // char
            branch_class.find("/b")!=std::string::npos || // unsigned char
            branch_class.find("/S")!=std::string::npos || // short
            branch_class.find("/s")!=std::string::npos || // unsigned short
            branch_class.find("/I")!=std::string::npos || // int
            branch_class.find("/i")!=std::string::npos || // unsigned int
            branch_class.find("/F")!=std::string::npos || // float
            branch_class.find("/D")!=std::string::npos || // double
            branch_class.find("/L")!=std::string::npos || // long
            branch_class.find("/l")!=std::string::npos || // unsigned long
            branch_class.find("/O")!=std::string::npos    // bool
        )
        {
            m_branch_type = BranchType::BUILTIN;
        }
        else
        {
            m_branch_type = BranchType::CLASS;
        }
    }

    template <typename T>
    void Handle<T>::Init(TTree& tree)
    {
        // no protection if the branch pointer is NULL
        // (so you can use this if the branch doesn't exist
        // as long as you don't call it).
        m_branch = tree.GetBranch(m_branch_name.c_str());
        if (m_branch)
        {
            if (m_branch_type == BranchType::NOT_SET)
            {
                std::string branch_class = m_branch->GetClassName();
                if (branch_class.empty())  // built in types don't have a ClassName --> using title
                {
                    branch_class = m_branch->GetTitle();
                }
                SetBranchType(branch_class);
            }
            switch (m_branch_type)
            {
                case BranchType::CLASS  : m_branch->SetAddress(&m_object_ptr); break;
                case BranchType::BUILTIN: m_branch->SetAddress(&m_object)    ; break;
                default: throw std::runtime_error("[CLASSNAME] ERROR: branch type not supported!"); 
            }
        }
    }

    template <typename T>
    const T& Handle<T>::get()
    {
        if (not IsLoaded())
        {
            Load();
        }
        switch (m_branch_type)
        {
            case BranchType::CLASS  : return *m_object_ptr;
            case BranchType::BUILTIN: return m_object;
            default: throw std::runtime_error("[CLASSNAME] ERROR: branch type not supported!"); 
        }
    }
    
} // namespace NAMESPACE


// CLASSNAME to handle all the branches for the TTree 
// ------------------------------------------------------------------------------------------------- //

class CLASSNAME
{
    public:
        CLASSNAME();

        // methods:
        void Init(TTree* const tree);
        void Init(TTree& tree);
        void GetEntry(const unsigned int entry);
		void LoadAllBranches();

TRIGGER_DEF_V1
        // static methods:
        static void progress(const int nEventsTotal, const int nEventsChain);
    
        // branch accessors
BRANCH_ACCESSORS_V1
    private:
    
        // handles
HANDLES
};


// public interface 
// ------------------------------------------------------------------------------------------------- //

namespace NAMESPACE
{
TRIGGER_DEF_V2
BRANCH_ACCESSORS_V2
} // namespace NAMESPACE


// ################################################################################################# //
// CMSSW complient interface 
// ################################################################################################# //
#else


// declaration of EdmHandleWrapper for CMSSW use
// ---------------------------------------------------------------------------------------------- //

#include "FWCore/Framework/interface/Event.h"
#include "DataFormats/Common/interface/Handle.h"

namespace NAMESPACE
{
    template <typename T>
    class EdmHandleWrapper
    {
        public:
			// construct:
            explicit EdmHandleWrapper(const std::string& label, const std::string& instance);

			// clear the handles
            void Clear();

			// set the handles to point to an event
            void SetEvent(const edm::Event& event_ptr);

			// get the data from the branch
            const T& get();

        private:
			// members:
            std::string m_label;
            std::string m_instance;
            const edm::Event* m_event_ptr;
            edm::Handle<T> m_handle;

            // no default constructor
            EdmHandleWrapper() {}
    };
    
    template <typename T>
    /*explicit*/ EdmHandleWrapper<T>::EdmHandleWrapper(const std::string& label, const std::string& instance)
        : m_label(label) 
        , m_instance(instance) 
        , m_event_ptr(NULL)
        , m_handle()
    {
    }

    template <typename T>
    void EdmHandleWrapper<T>::Clear()
    {
        m_handle.clear();
    }

    template <typename T>
    void EdmHandleWrapper<T>::SetEvent(const edm::Event& event)
    {
        m_event_ptr = &event;
        m_handle.clear();
    }

    template <typename T>
    const T& EdmHandleWrapper<T>::get()
    {
        if (!m_handle.isValid())
        {
            m_event_ptr->getByLabel(m_label, m_instance, m_handle);
            if (!m_handle.isValid())
            {
                const std::string error = Form("[CLASSNAME] ERROR: \\\"(%s, %s)\\\" input tag is invalid!", m_label.c_str(), m_instance.c_str());
                throw std::runtime_error(error);
            }
        }
        return *m_handle;
    }

} // end namespace NAMESPACE


// declaration of CLASSNAME for CMSSW use
// ---------------------------------------------------------------------------------------------- //

class CLASSNAME
{
    public:
        // construct:
        CLASSNAME();

        // methods:
        void Init();
        void SetEvent(const edm::Event&);

TRIGGER_DEF_V1
        // methods:
BRANCH_ACCESSORS_CMSSW_V1
        // static methods:
        static void progress(const int nEventsTotal, const int nEventsChain);

    private:
        // members:
        const edm::Event* m_event_ptr;

        // branches 
HANDLES_CMSSW
};

// public interface 
// ------------------------------------------------------------------------------------------------- //

namespace NAMESPACE
{
TRIGGER_DEF_V2
BRANCH_ACCESSORS_CMSSW_V2
} // namespace NAMESPACE

#endif //ifndef CMS2_USE_CMSSW

#ifndef __CINT__
extern CLASSNAME OBJNAME;
#endif


#endif // CLASSNAME_H
"""
	else:
		header_str = """#ifndef CLASSNAME_H
#define CLASSNAME_H

#include "Math/LorentzVector.h"
#include "Math/Point3D.h"
#include "TMath.h"
#include "TBranch.h"
#include "TTree.h"
#include "TH1F.h"
#include "TFile.h"
#include "TBits.h"
#include "TString.h"
#include <vector> 
#include <string>          

typedef ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<float> > LorentzVector;
typedef ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> > LorentzVectorD;

// Handle Classes to handle branch loading 
// ------------------------------------------------------------------------------------------------- //

namespace NAMESPACE
{
    struct BranchType
    {
        enum value_type
        {
            CLASS,
            BUILTIN,
            NOT_SET,

            static_size
        };
    };

    template <typename T>
    class Handle
    {
        public:
            
            // construct: 
            explicit Handle(const std::string& branch_name);

            // destroy:
            virtual ~Handle() {}

            // set the branch's entry 
            void GetEntry(const unsigned int entry);

            // initialize the handle's branches
            void Init(TTree& tree);

            // is the branch already loaded
            bool IsLoaded() const;

            // load the branch
            void Load();

            // clear the branch
            void Clear();

            // get the value
            const T& get();

        protected:

            // set the branch type private member (based on the TBranch) 
            void SetBranchType(const std::string& branch_class);

            // members:
            bool                   m_is_loaded;
            unsigned int           m_entry;
            std::string            m_branch_name;
            T*                     m_object_ptr;
            T                      m_object;
            BranchType::value_type m_branch_type;

            // the TTree owns the branch (this is only a reference pointer)
            TBranch*     m_branch;

        private:
            // no default constructor
            Handle() {}
    };

    template <typename T>
    /*explicit*/ Handle<T>::Handle(const std::string& branch_name)
        : m_is_loaded(false)
        , m_entry(0)
        , m_branch_name(branch_name)
        , m_object_ptr(NULL)
        , m_object()
        , m_branch_type(BranchType::NOT_SET)
        , m_branch(NULL)
    {
    }

    template <typename T>
    void Handle<T>::GetEntry(const unsigned int entry)
    {
        Clear();
        m_entry = entry;
    }

    template <typename T>
    bool Handle<T>::IsLoaded() const
    {
        return m_is_loaded;
    }
   
    template <typename T>
    void Handle<T>::Clear()
    {
        m_is_loaded = false;
    }

    template <typename T>
    void Handle<T>::Load()
    {
        if (m_branch)
        {
            m_branch->GetEntry(m_entry);
            m_is_loaded = true;
        }
        else
        {
            throw std::runtime_error("[CLASSNAME] ERROR: branch " + m_branch_name + " does not exist!"); 
        }
    }

    template <typename T>
    void Handle<T>::SetBranchType(const std::string& branch_class)
    {
        if (branch_class.find("edm::Wrapper")!=std::string::npos)
        {
            m_branch_type = BranchType::BUILTIN;
        }
        else if 
        (
            branch_class.find("/B")!=std::string::npos || // char
            branch_class.find("/b")!=std::string::npos || // unsigned char
            branch_class.find("/S")!=std::string::npos || // short
            branch_class.find("/s")!=std::string::npos || // unsigned short
            branch_class.find("/I")!=std::string::npos || // int
            branch_class.find("/i")!=std::string::npos || // unsigned int
            branch_class.find("/F")!=std::string::npos || // float
            branch_class.find("/D")!=std::string::npos || // double
            branch_class.find("/L")!=std::string::npos || // long
            branch_class.find("/l")!=std::string::npos || // unsigned long
            branch_class.find("/O")!=std::string::npos    // bool
        )
        {
            m_branch_type = BranchType::BUILTIN;
        }
        else
        {
            m_branch_type = BranchType::CLASS;
        }
    }

    template <typename T>
    void Handle<T>::Init(TTree& tree)
    {
        // no protection if the branch pointer is NULL
        // (so you can use this if the branch doesn't exist
        // as long as you don't call it).
        m_branch = tree.GetBranch(m_branch_name.c_str());
        if (m_branch)
        {
            if (m_branch_type == BranchType::NOT_SET)
            {
                std::string branch_class = m_branch->GetClassName();
                if (branch_class.empty())  // built in types don't have a ClassName --> using title
                {
                    branch_class = m_branch->GetTitle();
                }
                SetBranchType(branch_class);
            }
            switch (m_branch_type)
            {
                case BranchType::CLASS  : m_branch->SetAddress(&m_object_ptr); break;
                case BranchType::BUILTIN: m_branch->SetAddress(&m_object)    ; break;
                default: throw std::runtime_error("[CLASSNAME] ERROR: branch type not supported!"); 
            }
        }
    }

    template <typename T>
    const T& Handle<T>::get()
    {
        if (not IsLoaded())
        {
            Load();
        }
        switch (m_branch_type)
        {
            case BranchType::CLASS  : return *m_object_ptr;
            case BranchType::BUILTIN: return m_object;
            default: throw std::runtime_error("[CLASSNAME] ERROR: branch type not supported!"); 
        }
    }
    
} // namespace NAMESPACE


// CLASSNAME to handle all the branches for the TTree 
// ------------------------------------------------------------------------------------------------- //

class CLASSNAME
{
    public:
        CLASSNAME();

        // methods:
        void Init(TTree* const tree);
        void Init(TTree& tree);
        void GetEntry(const unsigned int entry);
		void LoadAllBranches();

TRIGGER_DEF_V1
       	// static methods:
       	static void progress(const int nEventsTotal, const int nEventsChain);

        // branch accessors
BRANCH_ACCESSORS_V1
    private:
    
        // handles
HANDLES
};


// public interface 
// ------------------------------------------------------------------------------------------------- //

namespace NAMESPACE
{
TRIGGER_DEF_V2
BRANCH_ACCESSORS_V2
} // namespace NAMESPACE

#ifndef __CINT__
extern CLASSNAME OBJNAME;
#endif


#endif // CLASSNAME_H
"""
	trigger_def_v1 = """        // trigger methods:
        bool passHLTTrigger(const TString& trigName);
        bool passL1Trigger(const TString& trigName);
"""
	trigger_def_v2 = """    bool passHLTTrigger(const TString& trigName);
    bool passL1Trigger(const TString& trigName);
"""

	if options.no_trig:
		header_str = header_str.replace("TRIGGER_DEF_V1", "")
		header_str = header_str.replace("TRIGGER_DEF_V2", "")
	else:
		header_str = header_str.replace("TRIGGER_DEF_V1", trigger_def_v1)
		header_str = header_str.replace("TRIGGER_DEF_V2", trigger_def_v2)
	header_str = header_str.replace("CLASSNAME"  , options.class_name)
	header_str = header_str.replace("OBJNAME"    , options.obj_name  )
	header_str = header_str.replace("NAMESPACE"  , options.namespace )

	branch_accessors_v1 = ""
	branch_accessors_v2 = ""
	handles = ""
	branch_accessors_cmssw_v1 = ""
	branch_accessors_cmssw_v2 = ""
	handles_cmssw = ""
	for branch_info in branch_infos:
		branch_accessors_v1 = branch_accessors_v1 + "        " + branch_info.GetAccessorDeclaration()
		branch_accessors_v2 = branch_accessors_v2 + "    "     + branch_info.GetAccessorDeclaration()
		handles             = handles             + "        " + branch_info.GetHandleDeclaration(options.namespace, False)
		if branch_info.IsEdmBranch():
			handles_cmssw             = handles_cmssw             + "        " + branch_info.GetHandleDeclaration(options.namespace, True)
			branch_accessors_cmssw_v1 = branch_accessors_cmssw_v1 + "        " + branch_info.GetAccessorDeclaration()
			branch_accessors_cmssw_v2 = branch_accessors_cmssw_v2 + "    "     + branch_info.GetAccessorDeclaration()

	if options.use_cmssw:
		header_str = header_str.replace("HANDLES_CMSSW"            , handles_cmssw            )
		header_str = header_str.replace("BRANCH_ACCESSORS_CMSSW_V1", branch_accessors_cmssw_v1)
		header_str = header_str.replace("BRANCH_ACCESSORS_CMSSW_V2", branch_accessors_cmssw_v2)

	header_str = header_str.replace("BRANCH_ACCESSORS_V1", branch_accessors_v1)
	header_str = header_str.replace("BRANCH_ACCESSORS_V2", branch_accessors_v2)
	header_str = header_str.replace("HANDLES"            , handles            )

	return header_str

# FIXME!! to not hard code path
def ImplString(branch_infos, use_cmssw):
	if (options.use_cmssw):
		impl_str = """#ifdef CMS2_USE_CMSSW
#include "CMS2/NtupleMacrosHeader/interface/CLASSNAME.h"
#else
#include "CLASSNAME.h"
#endif
#include <iostream>
#include <unistd.h>

// global object
CLASSNAME OBJNAME;

// ################################################################################################# //
// CMSSW independent
// ################################################################################################# //
#ifndef CMS2_USE_CMSSW

// implementation of CLASSNAME
// ---------------------------------------------------------------------------------------------- //

CLASSNAME::CLASSNAME()
HANDLES_CONSTRUCT
{
}

void CLASSNAME::Init(TTree& tree)
{
HANDLES_INIT
}

void CLASSNAME::Init(TTree* const tree)
{
    Init(*tree);
}

void CLASSNAME::GetEntry(const unsigned int entry)
{
HANDLES_GETENTRY
}

void CLASSNAME::LoadAllBranches()
{
HANDLES_LOADALLBRANCHES
}

// branch accessor methods:
BRANCH_ACCESSOR

TRIGGER_IMPL

/*static*/ void CLASSNAME::progress(const int nEventsTotal, const int nEventsChain)
{
    int period = 1000;
    if (nEventsTotal%1000 == 0)
    {
        // xterm magic from L. Vacavant and A. Cerri
        if (isatty(1))
        {
            if ((nEventsChain - nEventsTotal) > period)
            {
                float frac = (float)nEventsTotal/(nEventsChain*0.01);
                printf(\"\\015\\033[32m ---> \\033[1m\\033[31m%4.1f%%\\033[0m\\033[32m <---\\033[0m\\015\", frac);
                fflush(stdout);
            }
            else
            {
                printf(\"\\015\\033[32m ---> \\033[1m\\033[31m%4.1f%%\\033[0m\\033[32m <---\\033[0m\\015\", 100.0);
                std::cout << std::endl;
            }
        }
    }
}

namespace NAMESPACE
{
TRIGGER_WRAPPER
BRANCH_WRAPPER
} // namespace NAMESPACE



// ################################################################################################# //
// CMSSW complient
// ################################################################################################# //
#else

CLASSNAME::CLASSNAME()
HANDLES_CONSTRUCT_CMSSW
{
}

void CLASSNAME::Init()
{
HANDLES_CLEAR_CMSSW
}

void CLASSNAME::SetEvent(const edm::Event& event)
{
HANDLES_SETEVENT_CMSSW
}

TRIGGER_IMPL

/*static*/ void CLASSNAME::progress(const int nEventsTotal, const int nEventsChain)
{
    int period = 1000;
    if (nEventsTotal%1000 == 0)
    {
        // xterm magic from L. Vacavant and A. Cerri
        if (isatty(1))
        {
            if ((nEventsChain - nEventsTotal) > period)
            {
                float frac = (float)nEventsTotal/(nEventsChain*0.01);
                printf(\"\\015\\033[32m ---> \\033[1m\\033[31m%4.1f%%\\033[0m\\033[32m <---\\033[0m\\015\", frac);
                fflush(stdout);
            }
            else
            {
                printf(\"\\015\\033[32m ---> \\033[1m\\033[31m%4.1f%%\\033[0m\\033[32m <---\\033[0m\\015\", 100.0);
                std::cout << std::endl;
            }
        }
    }
}


// public interface implementation
// ---------------------------------------------------------------------------------------------- //
 
// branch accessor methods:
BRANCH_ACCESSOR_CMSSW

namespace NAMESPACE
{
TRIGGER_WRAPPER
BRANCH_WRAPPER_CMSSW
} // namespace NAMESPACE


#endif //CMS2_USE_CMSSW

"""
	else:
		if options.use_scram:
			impl_str = """#ifdef CMS2_USE_CMSSW
#include "CMS2/NtupleMacrosHeader/interface/CLASSNAME.h"
#else
#include "CLASSNAME.h"
#endif
"""
		else: 
			impl_str = "#include \"CLASSNAME.h\""
		impl_str = impl_str + """#include <iostream>
#include <unistd.h>

// global object
CLASSNAME OBJNAME;

// implementation of CLASSNAME
// ---------------------------------------------------------------------------------------------- //

CLASSNAME::CLASSNAME()
HANDLES_CONSTRUCT
{
}

void CLASSNAME::Init(TTree& tree)
{
HANDLES_INIT
}

void CLASSNAME::Init(TTree* const tree)
{
    Init(*tree);
}

void CLASSNAME::GetEntry(const unsigned int entry)
{
HANDLES_GETENTRY
}

void CLASSNAME::LoadAllBranches()
{
HANDLES_LOADALLBRANCHES
}

// branch accessor methods:
BRANCH_ACCESSOR

TRIGGER_IMPL

/*static*/ void CLASSNAME::progress(const int nEventsTotal, const int nEventsChain)
{
    int period = 1000;
    if (nEventsTotal%1000 == 0)
    {
        // xterm magic from L. Vacavant and A. Cerri
        if (isatty(1))
        {
            if ((nEventsChain - nEventsTotal) > period)
            {
                float frac = (float)nEventsTotal/(nEventsChain*0.01);
                printf(\"\\015\\033[32m ---> \\033[1m\\033[31m%4.1f%%\\033[0m\\033[32m <---\\033[0m\\015\", frac);
                fflush(stdout);
            }
            else
            {
                printf(\"\\015\\033[32m ---> \\033[1m\\033[31m%4.1f%%\\033[0m\\033[32m <---\\033[0m\\015\", 100.0);
                std::cout << std::endl;
            }
        }
    }
}
namespace NAMESPACE
{
TRIGGER_WRAPPER
BRANCH_WRAPPER
} // namespace NAMESPACE
"""
	trigger_impl = """// trigger methods:
bool CLASSNAME::passHLTTrigger(const TString& trigName)
{
    int trigIndx;
    std::vector<TString>::const_iterator begin_it = hlt_trigNames().begin();
    std::vector<TString>::const_iterator end_it = hlt_trigNames().end();
    std::vector<TString>::const_iterator found_it = find(begin_it, end_it, trigName);
    if(found_it != end_it)
    {
        trigIndx = found_it - begin_it;
    }
    else
    {
        std::cout << "Cannot find Trigger " << trigName << std::endl; 
        return false;
    }
    return hlt_bits().TestBitNumber(trigIndx);
}

bool CLASSNAME::passL1Trigger(const TString& trigName)
{
    int trigIndx;
    std::vector<TString>::const_iterator begin_it = l1_trigNames().begin();
    std::vector<TString>::const_iterator end_it = l1_trigNames().end();
    std::vector<TString>::const_iterator found_it = find(begin_it, end_it, trigName);
    if(found_it != end_it)
    {
        trigIndx = found_it - begin_it;
    }
    else 
    {
        std::cout << "Cannot find Trigger " << trigName << std::endl; 
        return false;
    }

    if (trigIndx <= 31)
    {
        unsigned int bitmask = 1;
        bitmask <<= trigIndx;
        return l1_bits1() & bitmask;
    }
    if (trigIndx >= 32 && trigIndx <= 63)
    {
        unsigned int bitmask = 1;
        bitmask <<= (trigIndx - 32); 
        return l1_bits2() & bitmask;
    }
    if (trigIndx >= 64 && trigIndx <= 95)
    {
        unsigned int bitmask = 1;
        bitmask <<= (trigIndx - 64); 
        return l1_bits3() & bitmask;
    }
    if (trigIndx >= 96 && trigIndx <= 127) 
    {
        unsigned int bitmask = 1;
        bitmask <<= (trigIndx - 96); 
        return l1_bits4() & bitmask;
    }
    return false;
}
"""
	trigger_wrapper = """    bool passHLTTrigger(const TString& trigName) {return OBJNAME.passHLTTrigger(trigName);}
	bool passL1Trigger(const TString& trigName) {return OBJNAME.passL1Trigger(trigName);}
"""

	if options.no_trig:
		impl_str = impl_str.replace("TRIGGER_IMPL"   , "")
		impl_str = impl_str.replace("TRIGGER_WRAPPER", "")
	else:
		impl_str = impl_str.replace("TRIGGER_IMPL"   , trigger_impl   )
		impl_str = impl_str.replace("TRIGGER_WRAPPER", trigger_wrapper)
	impl_str = impl_str.replace("CLASSNAME", options.class_name)
	impl_str = impl_str.replace("OBJNAME"  , options.obj_name)
	impl_str = impl_str.replace("NAMESPACE", options.namespace)

	handles_construct = ""
	handles_init = ""
	handles_getentry = ""
	handles_load_all_branches = ""
	branch_accessor = ""
	branch_wrapper = ""
	handles_construct_cmssw = ""
	handles_clear_cmssw = ""
	handles_setevent_cmssw = ""
	branch_accessor_cmssw = ""
	branch_wrapper_cmssw = ""
	is_first = True
	for branch_info in branch_infos:
		branch_accessor           = branch_accessor           + branch_info.GetAccessorDefinition(options.class_name)
		handles_init              = handles_init              + "    " + branch_info.GetInitCall()
		handles_getentry          = handles_getentry          + "    " + branch_info.GetEntryCall()
		handles_load_all_branches = handles_load_all_branches + "    " + branch_info.GetLoadAllBranchesCall()
		branch_wrapper            = branch_wrapper            + "    " + branch_info.GetAccessorWrapper(options.obj_name)
		if branch_info.IsEdmBranch():
			branch_accessor_cmssw  = branch_accessor_cmssw  + branch_info.GetAccessorDefinition(options.class_name)
			branch_wrapper_cmssw   = branch_wrapper_cmssw   + "    " + branch_info.GetAccessorWrapper(options.obj_name)
			handles_clear_cmssw    = handles_clear_cmssw    + "    " + branch_info.GetClearCall()
			handles_setevent_cmssw = handles_setevent_cmssw + "    " + branch_info.SetEventCall()

		if is_first:
			handles_construct = "    : " + branch_info.GetInitializer(False)
			if branch_info.IsEdmBranch():
				handles_construct_cmssw = "    : " + branch_info.GetInitializer(True)
		else:
			handles_construct = handles_construct + "\n    , " + branch_info.GetInitializer(False)
			if branch_info.IsEdmBranch():
				handles_construct_cmssw = handles_construct_cmssw + "\n    , " + branch_info.GetInitializer(True)
		is_first = False

	impl_str = impl_str.replace("HANDLES_CONSTRUCT_CMSSW", handles_construct_cmssw  )
	impl_str = impl_str.replace("BRANCH_ACCESSOR_CMSSW"  , branch_accessor_cmssw    )
	impl_str = impl_str.replace("BRANCH_WRAPPER_CMSSW"   , branch_wrapper_cmssw     )
	impl_str = impl_str.replace("HANDLES_CLEAR_CMSSW"    , handles_clear_cmssw      )
	impl_str = impl_str.replace("HANDLES_SETEVENT_CMSSW" , handles_setevent_cmssw   )
	impl_str = impl_str.replace("HANDLES_CONSTRUCT"      , handles_construct        )
	impl_str = impl_str.replace("HANDLES_INIT"           , handles_init             )
	impl_str = impl_str.replace("HANDLES_GETENTRY"       , handles_getentry         )
	impl_str = impl_str.replace("HANDLES_LOADALLBRANCHES", handles_load_all_branches)
	impl_str = impl_str.replace("BRANCH_ACCESSOR"        , branch_accessor          )
	impl_str = impl_str.replace("BRANCH_WRAPPER"         , branch_wrapper           )

	return impl_str


# simple function to test methods from branch_info 
# ---------------------------------------------------------------------------------- #

def TestMethods(branch_info):
	print branch_info
	print branch_info.GetAccessorName() 
	print branch_info.GetInitializer() 
	print branch_info.GetInitCall() 
	print branch_info.GetEntryCall() 
	print branch_info.GetAccessorDefinition(options.class_name) 
	print branch_info.GetAccessorDeclaration() 
	print branch_info.GetHandleDeclaration(options.namespace, False) 
	if branch_info.IsEdmBranch():
		print branch_info.GetHandleDeclaration(options.namespace, True) 
		print branch_info.GetAccessorDefinition(options.class_name, True) 

	branch_infos = list()
	branch_infos.append(branch_infos)
	print HeaderString(branch_infos, options.use_cmssw)
	print ImplString(branch_infos, True)
	return

# "main program" 
# ---------------------------------------------------------------------------------- #

def main():

	try:
		# check the validity of the options
		CheckOptions()

		# get the tree
		file = TFile.Open(options.file_name)
		if file.IsZombie():
			raise Exception("file is zombie")
		tree = file.Get(options.tree_name)	
		if not tree:
			raise ValueError("%s not found in %s" % (options.tree_name, options.file_name))

 		# create branch_infos 
		branch_infos = list() 
		aliases  = tree.GetListOfAliases()
		branches = tree.GetListOfBranches()
		if aliases and not aliases.IsEmpty():
 			for alias in aliases:
				branch = tree.GetBranch(alias.GetTitle())
				branch_info = BranchInfo(alias, branch)
				branch_infos.append(branch_info)
		else:
 			for branch in branches:
				alias = None
				branch_info = BranchInfo(alias, branch)
				branch_infos.append(branch_info)

		# print the header
		header_file = open("%s.h" % options.class_name, "w")
		header_file.write(HeaderString(branch_infos, options.use_cmssw))

		# print the header
		imple_file = open("%s.cc" % options.class_name, "w")
		imple_file.write(ImplString(branch_infos, options.use_cmssw))

		print "finished"
		return 0

	except Exception, e:
		print "[makeTTreeClassFiles] ERROR:", e
		return 1
	except:
		print "[makeTTreeClassFiles] ERROR:", sys.exc_info()[0]
		return 1

# do it
if __name__ == '__main__':
	main()

