/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of Willow Garage, Inc. nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>
#if !defined(WIN32)
  #include <sys/param.h>
#endif
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include "msgspec.h"
#include "utils.h"

#if defined(WIN32)
  #include <direct.h>
  #include <windows.h>
  #include <io.h>
  #define snprintf _snprintf
  #define mkdir(a,b) _mkdir(a)
  #define PATH_MAX MAX_PATH
  #define access _access
  #define F_OK 0x00
#endif

using std::string;

string tgt_dir;

class msgtest_gen
{
public:
  msgtest_gen() { }
  void process_file(const char *spec_file)
  {
    split_path(expand_path(spec_file), g_path, g_pkg, g_name);
    string cpp_dir = g_path + string("/test_cpp");
    tgt_dir = cpp_dir;
    if (access(cpp_dir.c_str(), F_OK))
      if (mkdir(cpp_dir.c_str(), 0755))
      {
        printf("Error from mkdir: [%s]\n", strerror(errno));
        exit(5);
      }

    if (access(tgt_dir.c_str(), F_OK) != 0)
      if (mkdir(tgt_dir.c_str(), 0755))
      {
        printf("Error from mkdir: [%s]\n", strerror(errno));
        exit(5);
      }

    msg_spec spec(spec_file, g_pkg, g_name, g_path);
    char fname[PATH_MAX];
    snprintf(fname, PATH_MAX, "%s/Test%s.cpp", tgt_dir.c_str(), g_name.c_str());
    FILE *f = fopen(fname, "w");
    if (!f)
    {
      printf("Couldn't write to %s\n", fname);
      exit(7);
    }

    string pkg_upcase = to_upper(g_pkg), msg_upcase = to_upper(g_name);
    fprintf(f, "#include <ctime>\n");
    fprintf(f, "#include \"../cpp/%s/%s.h\"\n", g_pkg.c_str(), g_name.c_str());
    fprintf(f, "\nnamespace ros {\nvoid msg_destruct() { }\n}\n\n");
    fprintf(f, "\nbool equals(const %s::%s &a, const %s::%s b)\n{\n",
            g_pkg.c_str(), g_name.c_str(), g_pkg.c_str(), g_name.c_str());
    fprintf(f, "  bool ok = true;\n");
    fprintf(f, "%s", spec.equals(string()).c_str());
    fprintf(f, "  return ok;\n");
    fprintf(f, "}\n\n");
    fprintf(f, "int main(int argc, char **argv)\n{\n");
    fprintf(f, "  srand(time(NULL));\n");
    fprintf(f, "  %s::%s a, b;\n", g_pkg.c_str(), g_name.c_str());
    fprintf(f, "%s", spec.test_populate("a").c_str());
    fprintf(f, "  uint32_t serlen = a.serializationLength();\n");
    fprintf(f, "  uint8_t *s = new uint8_t[serlen];\n");
    fprintf(f, "  uint8_t *eoser = a.serialize(s);\n");
    fprintf(f, "  if (eoser - s != serlen)\n"
               "    printf(\"expected serialization to take %%d bytes but "
                            "it took %%d bytes\\n\", serlen, eoser - s);\n");
    fprintf(f, "  uint8_t *eodeser = b.deserialize(s);\n");
    fprintf(f, "  if (eodeser - s != serlen)\n"
               "    printf(\"expected deserialization to take %%d bytes but "
                            "it took %%d bytes\\n\", serlen, eodeser - s);\n");
    fprintf(f, "  delete[] s;\n");
    fprintf(f, "  return (equals(a,b) ? 0 : 1);\n}\n\n");
    fclose(f);
  }
};



int main(int argc, char **argv)
{
  if (argc <= 1)
  {
    printf("usage: genmsgtest MSG1 [MSG2] ...\n");
    return 1;
  }
  msgtest_gen gen;
  for (int i = 1; i < argc; i++)
    gen.process_file(argv[i]);
  return 0;
}
