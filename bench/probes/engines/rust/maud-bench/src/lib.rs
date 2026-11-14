use common::{Article, Item, STYLE};
use maud::{html, Markup, PreEscaped, DOCTYPE};

pub fn simple(title: &str, body: &str) -> String {
    html! {
        title { (title) }
        h1 { (title) }
        div { (PreEscaped(body)) }
    }
    .into_string()
}

pub fn stocks(items: &[Item]) -> String {
    let page: Markup = html! {
        (DOCTYPE)
        html {
            head {
                title { "Stock Prices" }
                meta http-equiv="Content-Type" content="text/html; charset=UTF-8";
                meta http-equiv="Content-Style-Type" content="text/css";
                meta http-equiv="Content-Script-Type" content="text/javascript";
                link rel="shortcut icon" href="/images/favicon.ico";
                link rel="stylesheet" type="text/css" href="/css/style.css" media="all";
                script type="text/javascript" src="/js/util.js" {}
                style type="text/css" { (PreEscaped(STYLE)) }
            }
            body {
                h1 { "Stock Prices" }
                table {
                    thead {
                        tr {
                            th { "#" }
                            th { "symbol" }
                            th { "name" }
                            th { "price" }
                            th { "change" }
                            th { "ratio" }
                        }
                    }
                    tbody {
                        @for item in items {
                            tr class=(item.row_class) {
                                td { (item.index) }
                                td { a href={ "/stocks/" (item.value.symbol) } { (item.value.symbol) } }
                                td { a href=(item.value.url) { (item.value.name) } }
                                td { strong { (item.value.price) } }
                                @if item.negative_class.is_empty() {
                                    td { (item.value.change) }
                                    td { (item.value.ratio) }
                                } @else {
                                    td class="minus" { (item.value.change) }
                                    td class="minus" { (item.value.ratio) }
                                }
                            }
                        }
                    }
                }
            }
        }
    };
    page.into_string()
}

pub fn news(title: &str, articles: &[Article]) -> String {
    let page: Markup = html! {
        (DOCTYPE)
        html {
            head { title { (title) } }
            body {
                h1 { (title) }
                @for article in articles {
                    article {
                        h2 { a href=(article.url) { (article.title) } }
                        p class="meta" { (article.author) " - " (article.date) }
                        p { (article.teaser) }
                        ul { @for tag in &article.tags { li { (tag.name) } } }
                    }
                }
            }
        }
    };
    page.into_string()
}
