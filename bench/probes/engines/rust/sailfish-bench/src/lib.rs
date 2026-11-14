use common::{Article, Item};
use sailfish::TemplateSimple;

#[derive(TemplateSimple)]
#[template(path = "w1.stpl")]
pub struct Simple<'a> {
    pub title: &'a str,
    pub body: &'a str,
}

#[derive(TemplateSimple)]
#[template(path = "w2.stpl")]
pub struct Stocks<'a> {
    pub items: &'a [Item],
}

#[derive(TemplateSimple)]
#[template(path = "w3.stpl")]
pub struct News<'a> {
    pub title: &'a str,
    pub articles: &'a [Article],
}
