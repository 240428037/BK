


#include<iostream>
#include<mutex>
#include<mysql/mysql.h>
#include<jsoncpp/json/json.h>

using namespace std;

//参数定义为宏，方便后续更改
#define MYSQL_HOST "127.0.0.1"
#define MYSQL_USER "root"
#define MYSQL_PSWD ""
#define MYSQL_DB "db_blog"

//设计一个blog_system的命名空间-防止命名冲突
//包含TableBlog和TableTag两个类
namespace blog_system{
	//static修饰全局变量以及函数，作用是修改作用域，表示这个变量或函数，仅在本文件内生效
	static std::mutex g_mutex;//加锁保证线程安全，使每一个操作都是原子操作
	//向外提供接口返回初始化完成的mysql句柄(连接服务器，选择数据库，设置字符集)
	MYSQL*MysqlInit(){
		MYSQL*mysql;
		//初始化
		mysql = mysql_init(NULL);
		if (mysql == NULL){
			printf("init mysql error\n");
			return NULL;
		}
		//连接服务器
		//数据库名称NULL默认选择
		if (mysql_real_connect(mysql, MYSQL_HOST, MYSQL_USER, MYSQL_PSWD, NULL, 0, NULL, 0) == NULL){
			printf("connect mysql server error:%s\n", mysql_error(mysql));
			mysql_close(mysql);
			return NULL;
		}
		//设置字符集
		if (mysql_set_character_set(mysql, "utf8") != 0){
			printf("set client character error:%s\n", mysql_error(mysql));
			mysql_close(mysql);
			return NULL;
		}
		//选择数据库
		if (mysql_select_db(mysql, MYSQL_DB) != 0){
			printf("select db error:%s\n", mysql_error(mysql));
			mysql_close(mysql);
			return NULL;
		}
		return mysql;
	}


	//销毁句柄
	void MysqlRelease(MYSQL*mysql){
		if (mysql){
			mysql_close(mysql);
		}
		return;
	}

	//执行语句的共有接口
	bool MysqlQuery(MYSQL*mysql, const char*sql){
		int ret = mysql_query(mysql, sql);
		if (ret != 0){
			printf("query sql:[%s] failed:%s\n", sql, mysql_error(mysql));
			return false;
		}
		return true;
	}



	class TableBlog
	{
	public:
		TableBlog(MYSQL*mysql) :_mysql(mysql){}//构造函数 
		//从blog中取出博客信息，组织sql语句，将数据插入数据库;
		bool Insert(Json::Value &blog){
//id tag_id title content ctime
#define INSERT_BLOG "insert tb_blog values(null,'%d','%s','%s',now());"
			//因为博客正文长度不定，可能会很长，因此如果直接定义tmp空间的长度固定，有可能会越界访问
			int len = blog["content"].asString().size() + 4096;
			char *tmp = (char*)malloc(len);
			//int sprintf(char *str, const char *format, ...) 
			//发送格式化输出到str所指向的字符串
			sprintf(tmp, INSERT_BLOG, blog["tag_id"].asInt(),
				blog["title"].asCString(),
				blog["content"].asCString());
			bool ret = MysqlQuery(_mysql, tmp);
			free(tmp);
			return ret;
		}


		//根据博客id删除博客
		bool Delete(int blog_id){	
#define DELETE_BLOG "delete from tb_blog where id=%d;"
			char tmp[1024] = { 0 };
			sprintf(tmp, DELETE_BLOG, blog_id);
			bool ret = MysqlQuery(_mysql, tmp);
			return ret;
		}


		//从blog中取出博客信息，组织sql语句，更新数据库中的数据
		bool Update(Json::Value &blog){	
//id tag_id title content ctime
//id和ctime不可修改
#define UPDATE_BLOG "update tb_blog set tag_id=%d,title='%s',content='%s'where id=%d;"
			int len = blog["content"].asString().size() + 4096;
			char*tmp = (char*)malloc(len);
			sprintf(tmp, UPDATE_BLOG, blog["tag_id"].asInt(),
				blog["title"].asCString(),
				blog["content"].asCString(),
				blog["id"].asInt());
			bool ret = MysqlQuery(_mysql, tmp);
			free(tmp);
			return ret;
		}


		//通过blog返回所有的博客信息(因为返回列表展示，因此不包含正文)
		bool GetAll(Json::Value *blogs){		
#define GETALL_BLOG "select id,tag_id,title,ctime from tb_blog;"
			//执行查询语句
			g_mutex.lock();//加锁
			bool ret = MysqlQuery(_mysql, GETALL_BLOG);
			if (ret == false){
				g_mutex.unlock();//解锁
				return false;
			}
			//保存结果集
			MYSQL_RES*res = mysql_store_result(_mysql);
			g_mutex.unlock();//解锁
			if (res == NULL){
				printf("store all blog result failed:%s\n", mysql_error(_mysql));
				return false;
			}
			//遍历结果集
			int row_num = mysql_num_rows(res);
			for (int i = 0; i < row_num; i++){
				MYSQL_ROW row = mysql_fetch_row(res);
				Json::Value blog;
				blog["id"] = std::stoi(row[0]);
				blog["tag_id"] = stoi(row[1]);
				blog["title"] = row[2];
				blog["ctime"] = row[3];
				blogs->append(blog);//添加json数组元素,每一个博客都属于所有博客数组中的一个元素
			}
			mysql_free_result(res);
			return true;
		}

		//返回单个博客信息，包含正文
		bool GetOne(Json::Value *blog){		
#define GETONE_BLOG "select tag_id,title,content,ctime from tb_blog where id=%d;"
			char tmp[1024] = { 0 };
			sprintf(tmp, GETONE_BLOG, (*blog)["id"].asInt());
			g_mutex.lock();//加锁
			bool ret = MysqlQuery(_mysql, tmp);
			if (ret == false){
				g_mutex.unlock();//解锁
				return false;
			}
			//保存结果集
			MYSQL_RES*res = mysql_store_result(_mysql);
			g_mutex.unlock();//解锁
			if (res == NULL){
				printf("store once blog result failed:%s\n", mysql_error(_mysql));
				return false;
			}
			int row_num = mysql_num_rows(res);
			if (row_num != 1){
				printf("get one blog result error\n");
				mysql_free_result(res);
				return false;
			}
			//遍历结果集
			MYSQL_ROW row = mysql_fetch_row(res);
			//blog指针需要解引用
			(*blog)["tag_id"] = stoi(row[0]);
			(*blog)["title"] = row[1];
			(*blog)["content"] = row[2];
			(*blog)["ctime"] = row[3];
			mysql_free_result(res);
			return true;
		}
	private:
		MYSQL*_mysql;//操作句柄
	};



	class TableTag
	{
	public:
		TableTag(MYSQL*mysql) :_mysql(mysql){}
		bool Insert(Json::Value &tag){
#define INSERT_TAG "insert tb_tag values(null,'%s');"
			char tmp[1024] = { 0 };
			sprintf(tmp, INSERT_TAG, tag["name"].asCString());
			return MysqlQuery(_mysql, tmp);
		}
		bool Delete(int tag_id){
#define DELETE_TAG "delete from tb_tag where id=%d;"
			char tmp[1024] = { 0 };
			sprintf(tmp, DELETE_TAG, tag_id);
			return MysqlQuery(_mysql, tmp);
		}
		bool Update(Json::Value &tag){
#define UPDATE_TAG "update tb_tag set name='%s' where id=%d;"
			char tmp[1024] = { 0 };
			sprintf(tmp, UPDATE_TAG, tag["name"].asCString(), tag["id"].asInt());
			return MysqlQuery(_mysql, tmp);
		}
		bool GetAll(Json::Value *tags){
#define GETALL_TAG "select id,name from tb_tag;"
			g_mutex.lock();//加锁
			bool ret = MysqlQuery(_mysql, GETALL_TAG);
			if (ret == false){
				g_mutex.unlock();//解锁
				return false;
			}
			//保存结果集到本地
			MYSQL_RES *res = mysql_store_result(_mysql);
			g_mutex.unlock();//解锁
			if (res == NULL){
				printf("store all tag result failed:%s\n", mysql_error(_mysql));
				return false;
			}
			//遍历结果集
			int row_num = mysql_num_rows(res);
			for (int i = 0; i < row_num; i++){
				MYSQL_ROW row = mysql_fetch_row(res);
				Json::Value tag;
				tag["id"] = stoi(row[0]);
				tag["name"] = row[1];
				tags->append(tag);
			}
			mysql_free_result(res);
			return true;
		}
		bool GetOne(Json::Value *tag){
#define GETONE_TAG "select name from tb_tag where id=%d;"
			g_mutex.lock();//加锁
			char tmp[1024] = { 0 };
			sprintf(tmp, GETONE_TAG, (*tag)["id"].asInt());
			bool ret = MysqlQuery(_mysql, tmp);
			if (ret == false){
				g_mutex.unlock();//解锁
				return false;
			}
			MYSQL_RES *res = mysql_store_result(_mysql);
			g_mutex.unlock();//解锁
			if (res == NULL){
				printf("store one tag result failed:%s\n", mysql_error(_mysql));
				return false;
			}
			int row_num = mysql_num_rows(res);
			if (row_num != 1){
				printf("get one tag result error\n");
				mysql_free_result(res);
				return false;
			}
			MYSQL_ROW row = mysql_fetch_row(res);
			(*tag)["name"] = row[0];
			mysql_free_result(res);
			return true;
		}
	private:
		MYSQL *_mysql;
	};
}