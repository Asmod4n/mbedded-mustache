use common::{Article, Item, STYLE};

markup::define! {
    Simple<'a>(title: &'a str, body: &'a str) {
        title { @title }
        h1 { @title }
        div { @markup::raw(body) }
    }

    Stocks<'a>(items: &'a [Item]) {
        @markup::doctype()
        html {
            head {
                title { "Stock Prices" }
                meta[{"http-equiv"} = "Content-Type", content = "text/html; charset=UTF-8"];
                meta[{"http-equiv"} = "Content-Style-Type", content = "text/css"];
                meta[{"http-equiv"} = "Content-Script-Type", content = "text/javascript"];
                link[rel = "shortcut icon", href = "/images/favicon.ico"];
                link[rel = "stylesheet", type = "text/css", href = "/css/style.css", media = "all"];
                script[type = "text/javascript", src = "/js/util.js"] {}
                style[type = "text/css"] { @markup::raw(STYLE) }
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
                        @for item in items.iter() {
                            tr[class = &item.row_class] {
                                td { @item.index }
                                td { a[href = format!("/stocks/{}", item.value.symbol)] { @item.value.symbol } }
                                td { a[href = &item.value.url] { @item.value.name } }
                                td { strong { @item.value.price } }
                                @if item.negative_class.is_empty() {
                                    td { @item.value.change }
                                    td { @item.value.ratio }
                                } else {
                                    td[class = "minus"] { @item.value.change }
                                    td[class = "minus"] { @item.value.ratio }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    News<'a>(title: &'a str, articles: &'a [Article]) {
        @markup::doctype()
        html {
            head { title { @title } }
            body {
                h1 { @title }
                @for article in articles.iter() {
                    article {
                        h2 { a[href = &article.url] { @article.title } }
                        p[class = "meta"] { @article.author " - " @article.date }
                        p { @article.teaser }
                        ul { @for tag in article.tags.iter() { li { @tag.name } } }
                    }
                }
            }
        }
    }
}
