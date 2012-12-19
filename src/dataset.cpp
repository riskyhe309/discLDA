/*
 * Copyright (C) 2007 by
 * 
 * 	Xuan-Hieu Phan
 *	hieuxuan@ecei.tohoku.ac.jp or pxhieu@gmail.com
 * 	Graduate School of Information Sciences
 * 	Tohoku University
 *
 * GibbsLDA++ is a free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * GibbsLDA++ is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GibbsLDA++; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "constants.h"
#include "strtokenizer.h"
#include "dataset.h"

using namespace std;

int dataset::write_wordmap(string wordmapfile, mapword2id * pword2id) {
    FILE * fout = fopen(wordmapfile.c_str(), "w");
    if (!fout) {
	printf("Cannot open file %s to write!\n", wordmapfile.c_str());
	return 1;
    }    
    
    mapword2id::iterator it;
    fprintf(fout, "%d\n", pword2id->size());
    for (it = pword2id->begin(); it != pword2id->end(); it++) {
	fprintf(fout, "%s %d\n", (it->first).c_str(), it->second);
    }
    
    fclose(fout);
    
    return 0;
}

int dataset::read_wordmap(string wordmapfile, mapword2id * pword2id) {
    pword2id->clear();
    
    FILE * fin = fopen(wordmapfile.c_str(), "r");
    if (!fin) {
	printf("Cannot open file %s to read!\n", wordmapfile.c_str());
	return 1;
    }    
    
    char buff[BUFF_SIZE_SHORT];
    string line;
    
    fgets(buff, BUFF_SIZE_SHORT - 1, fin);
    int nwords = atoi(buff);
    
    for (int i = 0; i < nwords; i++) {
	fgets(buff, BUFF_SIZE_SHORT - 1, fin);
	line = buff;
	
	strtokenizer strtok(line, " \t\r\n");
	if (strtok.count_tokens() != 2) {
	    continue;
	}
	
	pword2id->insert(pair<string, int>(strtok.token(0), atoi(strtok.token(1).c_str())));
    }
    
    fclose(fin);
    
    return 0;
}

int dataset::read_wordmap(string wordmapfile, mapid2word * pid2word) {
    pid2word->clear();
    
    FILE * fin = fopen(wordmapfile.c_str(), "r");
    if (!fin) {
	printf("Cannot open file %s to read!\n", wordmapfile.c_str());
	return 1;
    }    
    
    char buff[BUFF_SIZE_SHORT];
    string line;
    
    fgets(buff, BUFF_SIZE_SHORT - 1, fin);
    int nwords = atoi(buff);
    
    for (int i = 0; i < nwords; i++) {
	fgets(buff, BUFF_SIZE_SHORT - 1, fin);
	line = buff;
	
	strtokenizer strtok(line, " \t\r\n");
	if (strtok.count_tokens() != 2) {
	    continue;
	}
	
	pid2word->insert(pair<int, string>(atoi(strtok.token(1).c_str()), strtok.token(0)));
    }
    
    fclose(fin);
    
    return 0;
}

int dataset::read_trndata(string dfile, string wordmapfile) {
    mapword2id word2id;
    
    FILE * fin = fopen(dfile.c_str(), "r");
    if (!fin) {
	printf("Cannot open file %s to read!\n", dfile.c_str());
	return 1;
    }   
    
    mapword2id::iterator it;    
    char buff[BUFF_SIZE_LONG];
    string line;
    
    // get the number of documents
    fgets(buff, BUFF_SIZE_LONG - 1, fin);
    M = atoi(buff);
    if (M <= 0) {
	printf("No document available!\n");
	return 1;
    }
    
    // allocate memory for corpus
    if (docs) {
	deallocate();
    } else {
	docs = new document*[M];
    }
    
    // set number of words to zero
    V = 0;
    Totalwords = 0;

    for (int i = 0; i < M; i++) {
	fgets(buff, BUFF_SIZE_LONG - 1, fin);
	line = buff;
	strtokenizer strtok(line, " \t\r\n");
    
    //number of words in document, including repeated words
	int length = strtok.count_tokens(); 

    Totalwords += length;   // total number of words in the corpus

	if (length <= 0) {
	    printf("Invalid (empty) document!\n");
	    deallocate();
	    M = V = 0;
	    return 1;
	}
	
	// allocate new document
	document * pdoc = new document(length);
	
	for (int j = 0; j < length; j++) {
	    it = word2id.find(strtok.token(j));
	    if (it == word2id.end()) {
		// word not found, i.e., new word
		pdoc->words[j] = word2id.size();
		word2id.insert(pair<string, int>(strtok.token(j), word2id.size()));
	    } else {
		pdoc->words[j] = it->second;
	    }
	}
	
	// add new doc to the corpus
	add_doc(pdoc, i);
    }
    
    fclose(fin);
    
    // write word map to file
    if (write_wordmap(wordmapfile, &word2id)) {
	return 1;
    }
    
    // update number of words
    V = word2id.size();
    
    return 0;
}

int dataset::read_newdata(string dfile, string wordmapfile) {
    mapword2id word2id;
    map<int, int> id2_id;
    
    read_wordmap(wordmapfile, &word2id);
    if (word2id.size() <= 0) {
	printf("No word map available!\n");
	return 1;
    }

    FILE * fin = fopen(dfile.c_str(), "r");
    if (!fin) {
	printf("Cannot open file %s to read!\n", dfile.c_str());
	return 1;
    }   

    mapword2id::iterator it;
    map<int, int>::iterator _it;
    char buff[BUFF_SIZE_LONG];
    string line;
    
    // get number of new documents
    fgets(buff, BUFF_SIZE_LONG - 1, fin);
    M = atoi(buff);
    if (M <= 0) {
	printf("No document available!\n");
	return 1;
    }
    
    // allocate memory for corpus
    if (docs) {
	deallocate();
    } else {
	docs = new document*[M];
    }
    _docs = new document*[M];
    
    // set number of words to zero
    V = 0;
    Totalwords = 0;

    for (int i = 0; i < M; i++) {
	fgets(buff, BUFF_SIZE_LONG - 1, fin);
	line = buff;
	strtokenizer strtok(line, " \t\r\n");
	int length = strtok.count_tokens();
    Totalwords += length;

	vector<int> doc;
	vector<int> _doc;
	for (int j = 0; j < length; j++) {
	    it = word2id.find(strtok.token(j));
	    if (it == word2id.end()) {
		// word not found, i.e., word unseen in training data
		// do anything? (future decision)
	    } else {
		int _id;
		_it = id2_id.find(it->second);
		if (_it == id2_id.end()) {
		    _id = id2_id.size();
		    id2_id.insert(pair<int, int>(it->second, _id));
		    _id2id.insert(pair<int, int>(_id, it->second));
		} else {
		    _id = _it->second;
		}
		
		doc.push_back(it->second);
		_doc.push_back(_id);
	    }
	}
	
	// allocate memory for new doc
	document * pdoc = new document(doc);
	document * _pdoc = new document(_doc);
	
	// add new doc
	add_doc(pdoc, i);
	_add_doc(_pdoc, i);
    }
    
    fclose(fin);
    
    // update number of new words
    V = id2_id.size();
    
    return 0;
}

// new version from anthonylife
int dataset::read_nv_data(string dfile, int dicnum, int docnum, int headernum){
    
    FILE * fin = fopen(dfile.c_str(), "r");
    if (!fin){
        printf("Cannot open file %s to read!\n", dfile.c_str());
        return 1;
    }

    char buff[BUFF_SIZE_LONG];
    //string line;
    
    // each line corresponding a document
    M = docnum;
    V = dicnum;
    if (M <= 0){
        printf("Specify an invalid number of documents\n");
        return 1;
    }

    if (docs) {
        deallocate();
    } else {
        docs = new document*[M];
    }

    // loop document
    string line;
    Totalwords = 0;
    for (int i = 0; i < M; i++){
        fgets(buff, BUFF_SIZE_LONG - 1, fin);
        line = buff;
	    strtokenizer strtok(line, " \t\r\n");

        // number of words in document, including repeated words
        int length = strtok.count_tokens();

        // line header
        headernum = 2;
        int uniq_wordlen = length - headernum;
        
        document * pdoc = new document(uniq_wordlen);
        pdoc->rate = atof(strtok.token(1).c_str());
        // skip header
        string temp_long, temp_short;
        string separator = ":"; 
        int idx, wd_id, wd_cnt;
        
        for (int j = 2; j < length; j++){
            temp_long = strtok.token(j);
            idx = temp_long.find(separator);
            if (idx < 1){
                printf("Input data file is invalid.\n");
                return 1;
            }
            temp_short = temp_long.substr(0, idx-1);
            wd_id = atoi(temp_short.c_str());
            temp_short = temp_long.substr(idx+1, temp_long.length());
            wd_cnt = atoi(temp_short.c_str());
            pdoc->words_id[j-2] = wd_id;
            pdoc->words_cnt[j-2] = wd_cnt;
            pdoc->totalwdcnt += wd_cnt;
        }
    
        // number of all words in corpus 
        Totalwords += pdoc->totalwdcnt;

        // allocation for repeated words storage
        idx = 0;
        pdoc->allocate(pdoc->totalwdcnt);
        for (int j = 0; j < pdoc->length; j++){
            for (int k = 0; k < pdoc->words_cnt[j]; k++){
                pdoc->words[idx] = pdoc->words_id[j];
                idx += 1;
            }
        }
        if (idx != pdoc->totalwdcnt){
            printf("Totalwdcnt: %d, idx: %d.\n", pdoc->totalwdcnt, idx);
            printf("Word count is wrong.\n");
            return 1;
        }
	    // add new doc to the corpus
	    add_doc(pdoc, i);
    }
    fclose(fin);

    return 0;
}


// new version from anthonylife
int dataset::read_nv_newdata(string dfile, int dicnum, int docnum, \
        int headernum){
    FILE * fin = fopen(dfile.c_str(), "r");
    if (!fin) {
	    printf("Cannot open file %s to read!\n", dfile.c_str());
	    return 1;
    }   
    
    // new document number setting
    M = docnum;
    V = dicnum;
    if (M <= 0 ){
        printf("Invalid document number setting!\n");
        return 1;
    }

    // allocate memory for corpus
    if (docs) {
	deallocate();
    } else {
	docs = new document*[M];
    }
    _docs = new document*[M];
    
    Totalwords = 0;
    char buff[BUFF_SIZE_LONG];
    string line;
    for (int i = 0; i < M; i++){
        fgets(buff, BUFF_SIZE_LONG - 1, fin);
        line = buff;
	    strtokenizer strtok(line, " \t\r\n");
    
        // number of words in document, including repeated words
        int length = strtok.count_tokens();

        // line header
        headernum = 2;
        int uniq_wordlen = length - headernum;
        
        document * pdoc = new document(uniq_wordlen);
        document * _pdoc = new document(uniq_wordlen);
        pdoc->rate = atof(strtok.token(1).c_str());
        _pdoc->rate = pdoc->rate;
        // skip header
        string temp_long, temp_short;
        string separator = ":"; 
        int idx, wd_id, wd_cnt;
    
        for (int j = 2; j < length; j++){
            temp_long = strtok.token(j);
            idx = temp_long.find(separator);
            if (idx < 1){
                printf("Input data file is invalid.\n");
                return 1;
            }
            temp_short = temp_long.substr(0, idx-1);
            wd_id = atoi(temp_short.c_str());
            temp_short = temp_long.substr(idx+1, temp_long.length());
            wd_cnt = atoi(temp_short.c_str());
            pdoc->words_id[j-2] = wd_id;
            _pdoc->words_id[j-2] = wd_id;
            pdoc->words_cnt[j-2] = wd_cnt;
            _pdoc->words_cnt[j-2] = wd_cnt;
            pdoc->totalwdcnt += wd_cnt;
            _pdoc->totalwdcnt += wd_cnt;
        }
	    
        // number of all words in corpus 
        Totalwords += pdoc->totalwdcnt;
        
        // allocation for repeated words storage
        idx = 0;
        pdoc->allocate(pdoc->totalwdcnt);
        _pdoc->allocate(_pdoc->totalwdcnt);
        for (int j = 0; j < pdoc->length; j++){
            for (int k = 0; k < pdoc->words_cnt[j]; k++){
                pdoc->words[idx] = pdoc->words_id[j];
                _pdoc->words[idx] = _pdoc->words_id[j];
                idx += 1;
            }
        }
        if (idx != pdoc->totalwdcnt){
            printf("Totalwdcnt: %d, idx: %d.\n", pdoc->totalwdcnt, idx);
            printf("Word count is wrong.\n");
            return 1;
        }
        
        // add new doc
	    add_doc(pdoc, i);
	    _add_doc(_pdoc, i);
    }
    return 0;
}

int dataset::read_newdata_withrawstrs(string dfile, string wordmapfile) {
    mapword2id word2id;
    map<int, int> id2_id;
    
    read_wordmap(wordmapfile, &word2id);
    if (word2id.size() <= 0) {
	printf("No word map available!\n");
	return 1;
    }

    FILE * fin = fopen(dfile.c_str(), "r");
    if (!fin) {
	printf("Cannot open file %s to read!\n", dfile.c_str());
	return 1;
    }   

    mapword2id::iterator it;
    map<int, int>::iterator _it;
    char buff[BUFF_SIZE_LONG];
    string line;
    
    // get number of new documents
    fgets(buff, BUFF_SIZE_LONG - 1, fin);
    M = atoi(buff);
    if (M <= 0) {
	printf("No document available!\n");
	return 1;
    }
    
    // allocate memory for corpus
    if (docs) {
	deallocate();
    } else {
	docs = new document*[M];
    }
    _docs = new document*[M];
    
    // set number of words to zero
    V = 0;
    Totalwords = 0;

    for (int i = 0; i < M; i++) {
	fgets(buff, BUFF_SIZE_LONG - 1, fin);
	line = buff;
	strtokenizer strtok(line, " \t\r\n");
	int length = strtok.count_tokens();
    Totalwords += length;

	vector<int> doc;
	vector<int> _doc;
	for (int j = 0; j < length - 1; j++) {
	    it = word2id.find(strtok.token(j));
	    if (it == word2id.end()) {
		// word not found, i.e., word unseen in training data
		// do anything? (future decision)
	    } else {
		int _id;
		_it = id2_id.find(it->second);
		if (_it == id2_id.end()) {
		    _id = id2_id.size();
		    id2_id.insert(pair<int, int>(it->second, _id));
		    _id2id.insert(pair<int, int>(_id, it->second));
		} else {
		    _id = _it->second;
		}
		
		doc.push_back(it->second);
		_doc.push_back(_id);
	    }
	}
	
	// allocate memory for new doc
	document * pdoc = new document(doc, line);
	document * _pdoc = new document(_doc, line);
	
	// add new doc
	add_doc(pdoc, i);
	_add_doc(_pdoc, i);
    }
    
    fclose(fin);
    
    // update number of new words
    V = id2_id.size();
    
    return 0;
}

void document::allocate(int totalwdcnt){
    words = new int[totalwdcnt];
}