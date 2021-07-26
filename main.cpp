 
#include "db.hpp"
#include"httplib.h"
using namespace httplib;
using namespace std;
//因为接口里都涉及到数据库的操作-定义两个指针table_blog和table_tag
blog_system::TableBlog*table_blog;
blog_system::TableTag*table_tag;

//具体对应业务的处理
//从请求中取出正文-正文就是提交的博客信息，以json格式的字符串形式组织的
//将json格式的字符串进行反序列化，得到各个博客信息
//将得到的博客信息插入到数据库中
void InsertBlog(const Request &req,Response &rsp){
	printf("InsertBlog\n");
  Json::Reader reader;
  Json::Value blog;
  Json::FastWriter writer;//和StyleWrite一个功能，使用稍有差别
  Json::Value errmsg;
  bool ret=reader.parse(req.body,blog);//将请求正文进行json反序列化-因为正文就是json格式的博客信息
  if(ret==false){
    printf("InsertBlog parse blog json failed!\n");
    rsp.status=400;//客户端格式错误
    errmsg["ok"]=false;
    errmsg["reason"]="parse blog json failed!";
    rsp.set_content(writer.write(errmsg),"application/json");//添加正文到rsp.body中
    return;
  }
  ret=table_blog->Insert(blog);//db.hpp中的TableBlog的接口Insert
  if(ret==false){
    printf("InsertBlog insert into database failed!\n");
    rsp.status=500;//服务器内部错误
    return;
  }
  rsp.status=200;//200ok
  return;
}
//删除博客操作
void DeleteBlog(const Request&req,Response &rsp){
  // /blog/123 /blog(\d+) req.matches[0]=/blog/123 req/matches[1]=123
  int blog_id=stoi(req.matches[1]);//blog_id是放在URL中path中
  bool ret=table_blog->Delete(blog_id);//Delete接口
  if(ret==false){
    printf("DeleteBlog delete into database failed!\n");
    rsp.status=500;
    return;
  }
  rsp.status=200;
  return;
}

void UpdateBlog(const Request&req,Response &rsp){ 
  int blog_id=stoi(req.matches[1]);//获取博客ID
  Json::Value blog;//对正文进行反序列化
  Json::Reader reader;
  bool ret=reader.parse(req.body,blog);
  if(ret==false){
    printf("UpdateBlog parse json failed!\n");
    rsp.status=400;
    return;
  }
  blog["id"]=blog_id;
  ret=table_blog->Update(blog);
  if(ret==false){
    printf("UpdateBlog update into database failed!\n");
    rsp.status=500;
    return;
  }
  rsp.status=200;
  return;
}
//获取所有博客信息
//从数据库中取出博客列表数据
void GetAllBlog(const Request&req,Response &rsp){
  Json::Value blogs;
  bool ret=table_blog->GetAll(&blogs);
  if(ret==false){
    printf("GetAllBlog select from database failed!\n");
    rsp.status=500;
    return;
  }
  //将数据进行json序列化,添加到rsp正文中 
  Json::FastWriter writer;
  rsp.set_content(writer.write(blogs),"application/json");
  rsp.status=200;
  return;
}

void GetOneBlog(const Request&req,Response &rsp){
  int blog_id = stoi(req.matches[1]);
  Json::Value blog;
  blog["id"] = blog_id;
  bool ret=table_blog->GetOne(&blog);
  if(ret==false){
    printf("GetOneBlog select from database failed!\n");
    rsp.status=500;
    return;
  }
  Json::FastWriter writer;
  rsp.set_content(writer.write(blog),"application/json");
  rsp.status=200;
  return;
}

void InsertTag(const Request&req,Response &rsp){ 
  Json::Reader reader;
  Json::Value tag;
  bool ret=reader.parse(req.body,tag);
  if(ret==false){
    printf("InsertTag parse tag json failed!\n");
    rsp.status=400;
    return;
  }
  ret=table_tag->Insert(tag);
  if(ret==false){
    printf("InsertTag insert into database failed!\n");
    rsp.status=500;
    return;
  }
  rsp.status=200;
  return;
}

void DeleteTag(const Request&req,Response &rsp){
  int tag_id=stoi(req.matches[1]);
  bool ret=table_tag->Delete(tag_id);
  if(ret==false){
    printf("DeleteTag delete into database failed!\n");
    rsp.status=500;
    return;
  }
  rsp.status = 200;
  return;
}

void UpdateTag(const Request&req,Response &rsp){
  int tag_id=std::stoi(req.matches[1]);
  Json::Value tag;
  Json::Reader reader;
  bool ret=reader.parse(req.body,tag);
  if(ret==false){
    printf("UpdateTag parse json failed!\n");
    rsp.status=400;
    return;
  }
  tag["id"]=tag_id;
  ret=table_tag->Update(tag);
  if(ret==false){
    printf("UpdateTag update into database failed!\n");
    rsp.status=500;
    return;
  }
  rsp.status=200;
  return;
}

void GetAllTag(const Request&req,Response &rsp){
  Json::Value tags;
  bool ret=table_tag->GetAll(&tags);
  if(ret==false){
    printf("GetAllTag select from database failed!\n");
    rsp.status=500;
    return;
  }
  Json::FastWriter writer;
  rsp.set_content(writer.write(tags),"application/json");
  rsp.status=200;
  return;
}
void GetOneTag(const Request&req,Response &rsp){
  int tag_id = stoi(req.matches[1]);
  Json::Value tag;
  tag["id"] = tag_id;
  bool ret=table_tag->GetOne(&tag);
  if(ret==false){
    printf("GetOneTag select from database failed!\n");
    rsp.status=500;
    return;
  }
  Json::FastWriter writer;
  rsp.set_content(writer.write(tag),"application/json");
  rsp.status=200;
  return;
}


int main(){
  //table_blog和table_tag就是数据库访问的操作对象，通过这两个对象完成对数据库的操作
  MYSQL*mysql=blog_system::MysqlInit();(mysql);
  table_blog=new blog_system::TableBlog(mysql);
  table_tag=new blog_system::TableTag(mysql);
  //业务处理模块
  Server server;
  // /index.html->./www/index.html
  //设置相对根目录的目的：当客户端请求静态文件资源时,httplib会直接根据路径读取文件数据进行响应
  server.set_base_dir("./www");//设置URL中的相对根目录，并且在请求/的时候，自动添加index.html
  //博客信息的增删改查
  //正则表达式-一种字符串的匹配规则
  //\d-匹配数字字符
  //+-表示匹配前边的字符一次或多次 
  //()-为了临时保存匹配数据
  ///blog/(\d+)-表示匹配以/blog/开头，后边跟了一个数字的字符串格式，并且临时保存后边的数字
  //R"()"取出括号中所有字符的特殊含义-c++11
  server.Post("/blog",InsertBlog);//不是函数调用，传递的是函数的地址
  server.Delete(R"(/blog/(\d+))",DeleteBlog);
  server.Put(R"(/blog/(\d+))",UpdateBlog);
  server.Get("/blog",GetAllBlog);
  server.Get(R"(/blog/(\d+))",GetOneBlog);
  //标签信息的增删改查
  server.Post("/tag",InsertTag);
  server.Delete(R"(/tag/(\d+))",InsertTag);
  server.Put(R"(/tag/(\d+))",UpdateTag);
  server.Get("/tag",GetAllTag);
  server.Get(R"(/tag/(\d+))",GetOneTag);
  server.listen("0.0.0.0",9000);
  return 0;
}


//#include "db.hpp"
//using namespace std;
//int main(){
//	test();
//	return 0;
//}
//void test(){
//	MYSQL*mysql = blog_system::MysqlInit();
//	blog_system::TableBlog table_blog(mysql);
//	Json::Value blog;
//	//测试增加
//		blog["tag_id"] = 4;
//	blog["title"] = "PHP";
//	blog["content"] = "PHPBEST";
//	table_blog.Insert(blog);
//	//测试删除
//		table_blog.Delete(4);
//	//测试修改
//		blog["id"] = 3;
//	blog["tag_id"] = 4;
//	blog["title"] = "PHP";
//	blog["content"] = "PHPBEST";
//	table_blog.Update(blog);
//	//测试查询getall
//		table_blog.GetAll(&blog);
//	Json::StyledWriter writer;
//	cout << writer.write(blog) << endl;
//	//测试查询getone
//		blog["id"] = 1;
//	table_blog.GetOne(&blog);
//	Json::StyledWriter writer;
//	cout << writer.write(blog) << endl;
//	//测试标签表的增删改查
//		blog_system::TableTag table_tag(mysql);
//	Json::Value tag;
//	//测试插入删除
//		tag["name"] = "C";
//	table_tag.Insert(tag);
//	table_tag.Delete(5);
//	//测试修改
//		tag["name"] = "C#";
//	tag["id"] = 1;
//	table_tag.Update(tag);
//	//测试查询getall
//		table_tag.GetAll(&tag);
//	Json::StyledWriter writer;
//	cout << writer.write(tag) << endl;
//	//测试查询getone
//		tag["id"] = 1;
//	table_tag.GetOne(&tag);
//	Json::StyledWriter writer;
//	cout << writer.write(tag) << endl;
//}

