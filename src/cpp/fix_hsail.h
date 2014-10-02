#include <string>
#include <string.h>
#include <regex.h>
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

typedef std::string (*Modifier)(char*, std::string, regmatch_t* match,int);
//Exact replace - only one match
static std::string Replace(char* input, std::string replace, regmatch_t* match, int number_matches) {
    return replace;
}


//Modify - special case atomicnoret_st_rel
static std::string ReplaceAtomicSt(char* input, std::string replace, regmatch_t* match, int number_matches) {
    if (number_matches != 5) {
        std::cout <<"Error for special case : atomicnoret_st_rel"<<std::endl;
    }
    std::string instruction(input, match[1].rm_so, match[1].rm_eo - match[1].rm_so);
    std::string size(input, match[2].rm_so, match[2].rm_eo - match[2].rm_so);
    std::string arg1(input, match[3].rm_so, match[3].rm_eo - match[3].rm_so);
    std::string arg2(input, match[4].rm_so, match[4].rm_eo - match[4].rm_so);

    std::string return_string;
    return_string.append("st_global_rel_u");
    return_string.append(size);
    return_string.append(" ");
    return_string.append(arg2);
    return_string.append(" ");
    return_string.append(",");
    return_string.append(" ");
    return_string.append(arg1);
    return_string.append(" ");
    return return_string;
}

static int FindNextPattern(const char *text, const char* pattern, regmatch_t* match, int number_matches){
    regex_t reg_pattern;
    int rc;

    if (0 != (rc = regcomp(&reg_pattern, pattern, REG_EXTENDED))) {
          printf("regcomp() failed, returning nonzero (%d)\n", rc);
          return rc;
    }
    return regexec(&reg_pattern, text, number_matches, match, 0);
}

static void IteratePatterns(std::string &text, std::string pattern, 
        std::string replace_string,
        Modifier pattern_modifier,
        int number_matches) {
    
    std::string modified_string;
    char *input_string = new char[text.length() + 1];
    memcpy(input_string, text.c_str(), text.length());
    input_string[text.length()] = '\0';
    uint input_length = strlen(input_string);
    uint read_offset = 0;
    
    while (read_offset < input_length) {
        //Change this to dynamic sizing
        regmatch_t match[5];
        if (FindNextPattern(input_string, pattern.c_str(), match, number_matches) != 0) {
            //Found no more patterns
            break;
        }
        //Append till the p
        std::string pre_modifier (input_string, 
                        0, 
                        match[0].rm_so);
        std::string modifier = pattern_modifier(input_string,
                        replace_string, match, 
                        number_matches);

        modified_string.append (pre_modifier);
        modified_string.append (modifier);
        read_offset += match[0].rm_eo;
        input_string += match[0].rm_eo;
    }
    //Append the reset of the string
    modified_string.append(input_string);
    text = modified_string;
}

static void ConvertHsail(std::string& hsail_text){
     typedef struct InstMap {
        std::string new_version;
        std::string old_version;
    }InstMap;
    
    //Check if it is 1.0. If it is 1.0 convert to 0.95
    regmatch_t match[3];
    const char* version_pattern = "\\bversion\\s([0-9]+)\\s*:\\s*([0-9]+)\\s*:"; 
    if (FindNextPattern(hsail_text.c_str(),version_pattern, match,3) !=  0){
        std::cout<<"Could not find version"<<std::endl;
        return;
    }
    std::string major_version = 
            hsail_text.substr((int)match[1].rm_so, (int)match[1].rm_eo - match[1].rm_so);
    std::string minor_version = 
            hsail_text.substr((int)match[2].rm_so, (int)match[2].rm_eo - match[2].rm_so);
    std::cout<<"Version "<<major_version<<":"<<minor_version<<std::endl;
    if (major_version == "0" && minor_version == "95"){
        return;
    }
    std::cout<<"Attempting to convert HSAIL to 0:95"<<std::endl;
    if (major_version == "1" && minor_version == "0") {
        std::cout<<"Converting 1.0 HSAIL to 0.95"<<std::endl;
    } else {
        std::cout<<"Warning: Only 1.0P HSAIL can be converted"<<std::endl;
    }

    const int num_of_instructions = 14;
    InstMap instruction_table[num_of_instructions] = {
        {version_pattern, "version 0:95:"},
        {"\\bbr", "brn"},
        {"\\bmemfence_scar_global\\(sys\\)", "sync"},
        {"\\bcbr_b1", "cbr"},
        {"\\batomic_cas_global_scar_sys", "atomic_cas_global"},
        {"\\batomic_add_global_scar_sys", "atomic_add_global"},
        {"\\batomic_exch_global_scar_sys", "atomic_exch_global"},
        {"\\batomic_ld_scacq_sys_b", "ld_global_acq_u"},
        {"\\batomicnoret_add_global_rlx_sys_u32", "atomicnoret_add_global_u32"},
        {"\\batomicnoret_max_global_rlx_sys_s32", "atomicnoret_max_global_s32"},
        {"\\batomicnoret_min_global_rlx_sys_s32", "atomicnoret_min_global_s32"},
        {"\\balign \\(4\\)", "align 4"},
        {"\\balign \\(8\\)", "align 8" },
        {"\\bbarrier", "barrier_fgroup"},
    };
    for (int i=0; i<num_of_instructions; i++) {
        IteratePatterns(hsail_text, 
                instruction_table[i].new_version, 
                instruction_table[i].old_version,
                Replace,
                1);
    }
    
    //Special case: atomicnoret_st_screl_sys_b arguments have to be reversed
    std::string atomic_no_ret_pattern ("(\\batomicnoret_st_screl_sys_b)([0-9]+)\\s*(\\[\\$[sd][0-9]+\\s*\\+\\s*[0-9]*\\]),\\s*(\\$[sd][0-9]+)");
    IteratePatterns(hsail_text, atomic_no_ret_pattern.c_str(),"Don't Care", ReplaceAtomicSt,5 );
}
