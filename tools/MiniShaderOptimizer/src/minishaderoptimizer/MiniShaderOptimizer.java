/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package minishaderoptimizer;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.util.Iterator;
import java.util.LinkedList;

/**
 *
 * @author Pap
 */
public class MiniShaderOptimizer {

    static class ReplacementRec {
        public ReplacementRec() {
        }
        public ReplacementRec(String n, String r, boolean e) {
            export = e;
            name = n;
            replacement = r;
        }
        String name;
        String replacement;
        boolean export;
    }
    
    static int accSize = 0;
    static int repSize = 0;
    
    static public class LocalNameGen {
//        static String[] usableVars = {"F", "I", "J", "K", "O", "Q", "U", "Y", "Z"};
        static String[] usableVars = {};
        
        public void GenName(String name, LinkedList<ReplacementRec> localList) {
            ReplacementRec r = new ReplacementRec();
            r.name = name;
            if(m_cnt < usableVars.length)
                r.replacement = usableVars[m_cnt];
            else {
                int cnt = m_cnt - usableVars.length;
                char c = 'l';
                c += (cnt / 10);
                r.replacement = "" + c + (cnt % 10);
            }
            localList.add(r);
            m_cnt++;
        }
        
        int m_cnt = 0;
    }
   
    
    static String removeClosures(String s, char c0, char c1) {
        String newLine = "";
        int n = 0;
        for(int i = 0; i < s.length(); i++) {
            if(s.charAt(i) == c0) 
                n++;
            else if(s.charAt(i) == c1)
                n--;
            else if(n == 0)
                newLine = newLine + s.charAt(i);
        }
        return newLine;
    }
    
    public static String[] LineToTokens(String line) {
        line = line.replaceAll("\\\\ncbuffer", "\\\\nLINEBREAKcbuffer");
        line = line.replaceAll("\\\\nstruct", "\\\\nLINEBREAKstruct");

//        System.out.println("LineTok:" + line);
        
        String splitLineTokens[] = line.split("\\\\nLINEBREAK");
        for(int ll = 0; ll < splitLineTokens.length; ll++) {
            line = splitLineTokens[ll];
            if(ll != 0)
                line = "\"" + line;
            if(ll != (splitLineTokens.length - 1)) {
                if(line.contains("#include"))
                    line += "\\n";
                line += "\"";
            }
            splitLineTokens[ll] = line;
        }        
        return splitLineTokens;
    }
    
    public static void extractReplacement(String fileName, LinkedList<ReplacementRec> localList, String localChar, int localListOffset, LocalNameGen nameGen) {
        File file = new File(fileName);
        try{
            BufferedReader reader = new BufferedReader(new FileReader(file));
            String line = null;
            while ((line = reader.readLine()) != null) {
                
                String splitLineTokens[] = LineToTokens(line);
/*                
                line = line.replaceAll("\\\\ncbuffer", "\\\\nLINEBREAKcbuffer");
                line = line.replaceAll("\\\\nstruct", "\\\\nLINEBREAKstruct");
                
                String splitLineTokens[] = line.split("\\\\nLINEBREAK");
*/
                for(int ll = 0; ll < splitLineTokens.length; ll++) {
                    line = splitLineTokens[ll];
/*                    
                    if(ll != 0)
                        line = "\"" + line;
                    if(ll != (splitLineTokens.length - 1))
                        line += "\"";
*/
                    if(line.contains("#define"))
                        continue;
                    try {
//                        System.out.println("SCanning line: >" + line + "<");
                        // search function declarations
                        if((line.matches(".*\"float[ 2-4] *[0-9a-zA-Z].*") || line.matches(".*\"bool[ 2-4] *[0-9a-zA-Z].*"))
                                && (line.endsWith(")\""))) {
    //                        line = line.split("\"")[1];
    //                        System.out.println(line);
                            String functionName = line.split("\"")[1].split(" ")[1].split("\\(")[0];
                            if(line.contains(functionName+"\\(")) {
//                                System.out.println("Line: " + line);
//                                System.out.println("Function: " + functionName);
                                continue;
                            }
    //                        System.out.println(functionName);
//                            if(functionName.length() > 3) 
                            if(nameGen != null)
                                nameGen.GenName(functionName, localList);
                            else
                            {
                                ReplacementRec r = new ReplacementRec();
                                r.name = functionName;
    //                            r.replacement = "f"+(list.size()- static_globalsListStart);
    //                            list.add(r);
                                r.replacement = localChar+((localList.size()+localListOffset));
                                localList.add(r);
                            }
                        }

    //                    if(line.contains("Texture2D")) 
                        {
                            String[] replacer = {"RWTexture2D", "Texture2D", "cbuffer", "SamplerState", "SamplerComparisonState", "struct", "TextureCube"};

                            String lineSafe = line;
                            line = line.split("\"")[1];
                            String[] lines = line.split(";");
                            for(int l = 0; l < lines.length; l++) {
                                line = lines[l];
                                for(int r = 0; r < replacer.length; r++) {
                                    if(line.contains(replacer[r])) {
                                        System.out.println("TextLine: " + line);
                                        String[] gparams = line.split(" ")[1].split(",");
                                        for(int i = 0; i < gparams.length; i++) {
                                            String name = gparams[i].split(":")[0];
                                            name = removeClosures(name, '[', ']');
                                            System.out.println("Replace \"" + replacer[r] + "\" : " + name);
//                                            if(name.length() > 3) 
                                            if(nameGen != null)
                                                nameGen.GenName(name, localList);
                                            else
                                            {
                                                ReplacementRec rec = new ReplacementRec();
                                                rec.name = name;
                                                rec.replacement = localChar+(localList.size()+localListOffset);
                                                localList.add(rec);
                                            }
                                        }
                                    }
                                }
                            }

                            line = lineSafe;
                            // scan closures
                            boolean contains = false;
                            for(int r = 0; r < replacer.length; r++)
                                contains |= line.contains(replacer[r]);
                            if(contains) {
                                
                                String[] cbuflines = line.split("\\{");
                                for(int l = 1; l < cbuflines.length; l++) {
                                    try {
                                        line = cbuflines[l];
    //                                    System.out.println("Closure: " + line);
                                        line = line.split("\\}")[0];
                                        String[] params = line.split(";");
                                        for(int i = 0; i < params.length; i++) {
                                            String name = params[i].split(" ")[1].split(":")[0];
        //                                    System.out.println("Cbuf: " + name);
//                                            if(name.length() > 3) 
                                            if(nameGen != null)
                                                nameGen.GenName(name, localList);
                                            else
                                            {
                                                ReplacementRec r = new ReplacementRec();
                                                r.name = name;
                                                r.replacement = localChar+(localList.size()+localListOffset);
                                                localList.add(r);
                                            }
                                        }
                                    } catch (Exception eee) {

                                    }
                                }

                            }

                            line = lineSafe;
                        }


    /*                    
                        if(line.contains("cbuffer")) {
                            String lineSafe = line;
                            String[] cbuflines = line.split("cbuffer");
                            for(int l = 0; l < cbuflines.length; l++) {
                                try {
                                    line = cbuflines[l];
                                    line = line.split("\\{")[1].split("\\}")[0];
                                    String[] params = line.split(";");
                                    for(int i = 0; i < params.length; i++) {
                                        String name = params[i].split(" ")[1].split(":")[0];
    //                                    System.out.println("Cbuf: " + name);
                                        if(name.length() > 3) {
                                            ReplacementRec r = new ReplacementRec();
                                            r.name = name;
                                            r.replacement = "q"+localList.size();
                                            localList.add(r);
                                        }
                                    }
                                } catch (Exception eee) {

                                }
                            }
                            line = lineSafe;
                        }
    */                    
                        if(line.contains("static const")) {
                            String[] tmpToks = line.split(" ");
                            line = tmpToks[tmpToks.length-1].split(";")[0];

                            // remove parenthesis
                            line = removeClosures(line, '(', ')');
                            line = removeClosures(line, '{', '}');
    //                        System.out.println("Static Complete" + line);
                            String[] params = line.split(",");
                            for(int i = 0; i < params.length; i++) {
                                String name = params[i].split("=")[0];
                                name = removeClosures(name, '[', ']');

    //                            System.out.println("Static: " + name);
//                                if(name.length() > 3) 
                                if(nameGen != null)
                                    nameGen.GenName(name, localList);
                                else
                                {
                                    ReplacementRec r = new ReplacementRec();
                                    r.name = name;
                                    r.replacement = localChar+(localList.size()+localListOffset);
                                    localList.add(r);
                                }
                            }

                        }

                    } catch (Exception ee) {

                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        
        
        System.out.println("Extracted from " + fileName + ":");
        for(int i = 0; i < localList.size(); i++)
            System.out.println(" -> [" + localList.get(i).name + "]");
    }

    public static void translateReplacement(String fileName, LinkedList<ReplacementRec> list, String outFileName, int pass) {
        File file = new File(fileName);
        File outFile = new File(outFileName);
        try{
            BufferedWriter writer = new BufferedWriter(new FileWriter(outFile));
            BufferedReader reader = new BufferedReader(new FileReader(file));
            String line = null;
            while ((line = reader.readLine()) != null) {
                    
                String splitLineTokens[] = LineToTokens(line);
                for(int ll = 0; ll < splitLineTokens.length; ll++) {
                    line = splitLineTokens[ll];
                    
                    if(outFileName.endsWith("globals.hpp") && line.contains("#define") ) {
                        if(pass == 0)
                            continue;
                        Iterator<ReplacementRec> iter = list.iterator();
                        while(iter.hasNext()) {
                            ReplacementRec r = iter.next();
                            if(r.export)
                                writer.write(
                                            "\"#define " + r.replacement + " " + r.name + "\\n\"\n" 
                                        );
                        }
                        while(line.contains("#define"))
                            line = reader.readLine();

                    }
                    if(line.contains("\"")) {
                        Iterator<ReplacementRec> iter = list.iterator();
                        while(iter.hasNext()) {
                            ReplacementRec r = iter.next();
                            try {
                                line = line.replaceAll("\\b" + r.name + "\\b", r.replacement);
                            } catch (Exception ee) {

                            }
                        }
                    }
                    writer.write(line + "\n");
                }
            }
            reader.close();
            writer.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    static int static_globalsListStart;
    
    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        // TODO code application logic here
//        String path = "e:/BrainControl/SVN/code/eshared/engine/shaders";
//        String pathInput = "e:/BrainControl/SVN/binary/backup";
        String path = ".";
        String pathInput = path;
        File folder = new File(path);
        File[] listOfFiles = folder.listFiles(); 
        LinkedList<ReplacementRec> list = new LinkedList<ReplacementRec>();
        

                
//        list.add(new ReplacementRec("normalMtx", "N", true));
//        list.add(new ReplacementRec("TEXCOORD0", "T0", true));


        

//        list.add(new ReplacementRec("cbuffer", "H0", true));
/*
        list.add(new ReplacementRec("POSITION0", "P0", true));
        list.add(new ReplacementRec("COLOR0", "C0", true));
        list.add(new ReplacementRec("TEXCOORD0", "T0", true));
*/        
        
        
        list.add(new ReplacementRec("packoffset", "H5", true));
        list.add(new ReplacementRec("GetDimensions", "H4", true));
        list.add(new ReplacementRec("SamplerState", "H3", true));
        list.add(new ReplacementRec("numthreads", "H2", true));
        list.add(new ReplacementRec("TextureCube", "H1", true));
        list.add(new ReplacementRec("SamplerComparisonState", "H0", true));
        
        list.add(new ReplacementRec("RWTexture2D", "h9", true));
        list.add(new ReplacementRec("Texture2D", "h8", true));
        list.add(new ReplacementRec("floor", "h7", true));
        list.add(new ReplacementRec("ShaderOutput", "h6", true));
        list.add(new ReplacementRec("ShaderInput", "h5", true));
        list.add(new ReplacementRec("main", "h4", true));
        list.add(new ReplacementRec("register", "h3", true));
        list.add(new ReplacementRec("smoothstep", "h2", true));
        list.add(new ReplacementRec("saturate", "h1", true));
        list.add(new ReplacementRec("normalize", "h0", true));
        list.add(new ReplacementRec("length", "g9", true));
        list.add(new ReplacementRec("const", "g8", true));
        list.add(new ReplacementRec("return", "g7", true));
        
        list.add(new ReplacementRec("float2x3", "F6", true));
        list.add(new ReplacementRec("float3x3", "F5", true));
        list.add(new ReplacementRec("float4x4", "F4", true));
        list.add(new ReplacementRec("float4", "F3", true));
        list.add(new ReplacementRec("float3", "F2", true));
        list.add(new ReplacementRec("float2", "F1", true));
        list.add(new ReplacementRec("float", "F0", true));
        
        
        list.add(new ReplacementRec("(3.14159265f)","P1", true));
        list.add(new ReplacementRec("(2.f*3.14159265f)", "P2", true));
        list.add(new ReplacementRec("ePI","P1", false));
        list.add(new ReplacementRec("eTWOPI", "P2", false));
        list.add(new ReplacementRec("eCBI_CAMERA", "b0", false));
        list.add(new ReplacementRec("eCBI_LIGHT", "b1", false));
        list.add(new ReplacementRec("eCBI_MATERIAL", "b2", false));
        list.add(new ReplacementRec("eCBI_FX_PARAMS", "b3", false));
        list.add(new ReplacementRec("eCBI_PASS_AMBIENT", "b4", false));
        list.add(new ReplacementRec("eCBI_PASS_SHADOW", "b5", false ));
        static_globalsListStart = list.size();
        
        
        LinkedList<ReplacementRec> globalLocalList = new LinkedList<ReplacementRec>();
        extractReplacement(pathInput + "/globals.hpp", globalLocalList, "w", -globalLocalList.size(), null);
        for(int i = 0; i < globalLocalList.size(); i++)
            System.out.println("Global Conv: " + globalLocalList.get(i).name + " -> " + globalLocalList.get(i).replacement);
        translateReplacement(pathInput + "/globals.hpp", globalLocalList, path + "/globals.tst", 0);
        
        // extract replacements
        for (int i = 0; i < listOfFiles.length; i++) {
          if (listOfFiles[i].isFile()) {
              LocalNameGen nameGen = new LocalNameGen();
              String fileName = pathInput + "/" + listOfFiles[i].getName();
              String fileNameTmp = path + "/" + listOfFiles[i].getName();
              String tempFileName = fileNameTmp.substring(0, fileNameTmp.length() - 4) + ".tmp";
              if (fileName.endsWith(".hpp")) {
                  LinkedList<ReplacementRec> localList = new LinkedList<ReplacementRec>();
                  localList.addAll(list);
                  localList.addAll(globalLocalList);
                  
                  extractReplacement(fileName, localList, "l", -localList.size(), nameGen);                  
/*                  
                  if(fileName.endsWith("globals.hpp")) {
                      localList.clear();
                      localList.addAll(globalLocalList);
                  }
*/                 
                  translateReplacement(fileName, localList, tempFileName, 0);
              }
          }
        }
        
        // convert
        for (int i = 0; i < listOfFiles.length; i++) {
          if (listOfFiles[i].isFile()) {
              String fileName = path + "/" + listOfFiles[i].getName();
              fileName = fileName.substring(0, fileName.length() - 4) + ".tmp";
              String outFileName = fileName.substring(0, fileName.length() - 4) + ".hpp";
              if (listOfFiles[i].getName().endsWith(".hpp")) {
                  translateReplacement(fileName, list, outFileName, 1);
/*                  
                  LinkedList<ReplacementRec> tmp = new LinkedList<ReplacementRec>();
                  boolean testAll = !listOfFiles[i].getName().endsWith("globals.hpp");
                  int p = 0;
                  Iterator<ReplacementRec> iter = list.iterator();
                  while(iter.hasNext()) {
                      ReplacementRec r = iter.next();
                      p++;
                      if(testAll || (p < static_globalsListStart))
                          tmp.add(r);
                  }
                  
                  translateReplacement(fileName, tmp, outFileName, 1);
*/
              }
          }
        }
    }
}
