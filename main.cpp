#include <phpcpp.h>
#include <iostream>
#include <curl/curl.h>
#include <string>
#include <sys/time.h>
#include <mysql/mysql.h>
#include <unistd.h>

size_t curl_writer(void *buffer, size_t size, size_t count, void * stream)
{
    std::string * pStream = static_cast<std::string *>(stream);
    (*pStream).append((char *)buffer, size * count);

    return size * count;
};

/**
 * Generate a easy curl object, to carry out some simple set operation
 */
CURL * curl_easy_handler(const std::string & sUrl,
                         const std::string & sProxy,
                         std::string & sRsp,
                         unsigned int uiTimeout,
                         const std::string & sUserAgent,
                         bool usePost = false,
                         const std::string & postData = ""
                         )
{
    CURL * curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, sUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

    if (uiTimeout > 0) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, uiTimeout);
    }
    // Check use proxy
    if (!sProxy.empty()) {
        curl_easy_setopt(curl, CURLOPT_PROXY, sProxy.c_str());
    }
    // Set user agent
    curl_easy_setopt(curl, CURLOPT_USERAGENT, sUserAgent.c_str());

    // if use post, set post data
    if (usePost) {
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, postData.size());
    }

    // write function //
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_writer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sRsp);

    return curl;
}

/**
 * Uses the select function to monitor multi curl file descriptor state
 * if SUCCESS return 0 else return -1.
 */
int curl_multi_select(CURLM * curl_m)
{
    int ret = 0;

    struct timeval timeout_tv;
    fd_set  fd_read;
    fd_set  fd_write;
    fd_set  fd_except;
    int     max_fd = -1;

    FD_ZERO(&fd_read);
    FD_ZERO(&fd_write);
    FD_ZERO(&fd_except);

    // Set select timeout  //
    timeout_tv.tv_sec = 1;
    timeout_tv.tv_usec = 0;

    curl_multi_fdset(curl_m, &fd_read, &fd_write, &fd_except, &max_fd);

    /**
     * When max_fd returns with -1,
     * you need to wait a while and then proceed and call curl_multi_perform anyway.
     * How long to wait? I would suggest 100 milliseconds at least,
     * but you may want to test it out in your own particular conditions to find a suitable value.
     */
    if (-1 == max_fd)
    {
        //return -1;
    }

    int ret_code = ::select(max_fd + 1, &fd_read, &fd_write, &fd_except, &timeout_tv);
    switch (ret_code) {
	    case -1:
	        /* select error */
	        ret = -1;
	        break;
	    case 0:
	        /* select timeout */
	    default:
	        /* one or more of curl's file descriptors say there's data to read or write*/
	        ret = 0;
	        break;
    }

    return ret;
}

/*
 * Curl multi request
 * params like :
 * array(array('url' => 'http://sae.sina.com.cn', 'method' => 'post', 'postdata' => array('aa' => 'test')),
 *       array('url' => 'http://sae.sina.com.cn', 'method' => 'post', 'postdata' => array('aa' => 'tes2')),
 * ); 
 * @author lazypeople<hfutming@gmail.com>
 */
Php::Value mcurl(Php::Parameters &params)
{
    CURLM * curl_m = curl_multi_init();
    std::string PROXY = "";
    std::string USERAGENT = Php::ini_get("loafer.curl_useragent");
    int TIMEOUT = Php::ini_get("loafer.curl_timeout"); /* ms */
    unsigned int size = params[0].size();
    std::string RspArray[size];
    CURL * CurlArray[size];
    auto m = params[0].mapValue();
    std::string postdata[size];

    int num = 0;
    for(auto &i: m) {
    	auto each_piece = i.second.mapValue();
    	// Check wether contain keys `url`
    	std::string url;
    	if (each_piece.find("url") == each_piece.end()) {
    		throw Php::Exception("Must contain param url");
    	} else {
    		url = each_piece["url"].stringValue();
    	}
    	std::string method = "get";
    	// get request method
    	if (each_piece.find("method") == each_piece.end()) {
    	} else {
    		method = each_piece["method"].stringValue();
    	}
        bool use_post = false;
    	// Get post data
    	if (method == "post") {
            use_post = true;
    		if (each_piece.find("postdata") == each_piece.end()) {
	    		use_post = false;
	    	} else {
                std::string every;
                // dump post data
                for (auto &post_value : each_piece["postdata"]) {
                    every = every + post_value.first.stringValue() + "=" + post_value.second.stringValue() + "&";
                }
                postdata[num] = every;
	    	}
    	}
    	CurlArray[num] = NULL;
        CurlArray[num] = curl_easy_handler(url, PROXY, RspArray[num], TIMEOUT, USERAGENT, use_post, postdata[num]);
        if (CurlArray[num] == NULL)
        {
            throw Php::Exception("Start curl setting error");
        }
        curl_multi_add_handle(curl_m, CurlArray[num]);
    	num++;
    }
   
    int running_handles;
    while (CURLM_CALL_MULTI_PERFORM == curl_multi_perform(curl_m, &running_handles)) {
        //std::cout << running_handles << std::endl;
    }

    while (running_handles) {
        if (-1 == curl_multi_select(curl_m)) {
            throw Php::Exception("select error");
            break;
        } else {
            while (CURLM_CALL_MULTI_PERFORM == curl_multi_perform(curl_m, &running_handles)) {
            }
        }
    }

    int msgs_left;
    CURLMsg * msg;
    Php::Value array;
    while ((msg = curl_multi_info_read(curl_m, &msgs_left))) {
        if (CURLMSG_DONE == msg->msg) {
            int idx;
            for (idx = 0; idx < num; ++idx) {
                if (msg->easy_handle == CurlArray[idx]) break;
            }

            if (idx == num) {
                throw Php::Exception("curl not found");
            } else {
                array[idx] = RspArray[idx];
            }
        }
    }

    for (int idx = 0; idx < num; ++idx) {
        curl_multi_remove_handle(curl_m, CurlArray[idx]);
    }

    for (int idx = 0; idx < num; ++idx) {
        curl_easy_cleanup(CurlArray[idx]);
    }

    curl_multi_cleanup(curl_m);
    return array;
}

/*
 * Support many sql run at same time WITHIN A transaction
 * NOTICE::Operating tables must all be InnoDB
 * @author lazypeople<hfutming@gmail.com>
 */
Php::Value msql(Php::Parameters &params)
{
    // Get mysql connection info from php.ini
    std::string host = Php::ini_get("loafer.host");
    int port = Php::ini_get("loafer.port");
    std::string user = Php::ini_get("loafer.user");
    std::string pass = Php::ini_get("loafer.pass");
    std::string dbname = Php::ini_get("loafer.dbname");
    auto m = params[0].mapValue();
    if (host.empty()  or user.empty() or pass.empty()) {
        throw Php::Exception("Can not load mysql connection info from php.ini");
    }
    // try connect database
    MYSQL *mysql_;
    mysql_ = mysql_init(0);
    if (!mysql_real_connect(mysql_, host.c_str(), user.c_str(), pass.c_str(), dbname.c_str(), port, NULL, 0)) {
        mysql_close(mysql_);
        mysql_ = 0;
        throw Php::Exception("Can not connect to mysql");
    }
    // Close auto commit
    mysql_autocommit(mysql_, 0);
    // get sql from params
    mysql_query(mysql_, "START TRANSACTION");
    for (auto &sql:m) {
        //sql_combine = sql_combine + sql.second.stringValue() + ";";
        mysql_query(mysql_, sql.second.stringValue().c_str());
    }
    Php::Value retval;
    // Start tansfer action
    int commit_ret = mysql_query(mysql_, "COMMIT");
    if (commit_ret != 0) {
        // Roll back
        mysql_query(mysql_, "ROLLBACK");
        // Set return value
        retval["errorcode"] = 1;
        retval["errormsg"] = mysql_error(mysql_);
    }
    retval["errorcode"] = 0;
    retval["errormsg"] = "SUCCESS";
    // Free mysql connection 
    mysql_close(mysql_);
    mysql_ = NULL;
    return retval;
}

extern "C" {
    PHPCPP_EXPORT void *get_module() {
        static Php::Extension extension("loafer", "1.0");
        extension.add("mcurl", mcurl, {
        	Php::ByVal("mcurl", Php::Type::Array, true)
        });
        extension.add("msql", msql, {
            Php::ByVal("msql", Php::Type::Array, true)
        });
        extension.add(Php::Ini("loafer.host", "127.0.0.1"));
        extension.add(Php::Ini("loafer.user", "user"));
        extension.add(Php::Ini("loafer.pass", "pass"));
        extension.add(Php::Ini("loafer.port", 3306));
        extension.add(Php::Ini("loafer.dbname", "test"));
        extension.add(Php::Ini("loafer.curl_useragent", "Loafer Web Multi Request Agent"));
        extension.add(Php::Ini("loafer.curl_timeout", 2000));
        return extension;
    }
}