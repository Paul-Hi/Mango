---
title: Home
subtitle: This is the home of Mango!
layout: wrap
sidebar: home
url: index.html
---


<div class="d-flex flex-column justify-content-between sidebar-mango col-lg-1">
    <div class="card mt-3 flex-grow-1">
        <div class="card-body">
            <h1 class="card-title">{{ page.title }}</h1>
            <p class="card-text">{{ page.subtitle }}</p>
            <h3 class="card-text">This page is in an early stage and work in progress!</h3>
        </div>
    </div>
    <div class="card mt-3 flex-grow-1">
        <div class="card-body">
            <h1 class="card-title">Latest Posts</h1>
            <p class="card-text">
                {% for post in site.posts limit:3 %}
                <a class="mango-card-link d-flex p-2" href="{{ post.url | relative_url }}">
                    <span class="pl-2"> {{ post.title}} </span>
                    <span class="post-date ml-auto pl-2"> {{ post.date | date: "%b %-d, %Y" }}</span>
                </a>
                {% endfor %}
            </p>
        </div>
    </div>
    <div class="card mt-3 flex-grow-1">
        <div class="card-body">
            <h1 class="card-title">News</h1>
            <p class="card-text">
                {% include news.html %}
            </p>
        </div>
    </div>
</div>

<div class="d-flex flex-column justify-content-between col">
    <div class="card mt-3 flex-grow-1">
        <div class="card-body">
            <h1 class="card-title">Mango</h1>
            <p class="card-text">
                Mango is an Open Source Framework. Or at least it should be the future.
                A playground for people like me.
            </p>
        </div>
    </div>
    <div class="card mt-3 flex-grow-1">
        <div class="card-body">
            <h1 class="card-title text-center">Gallery</h1>
            <div id="mango-images" class="carousel slide carousel-fade p-0"
                data-ride="carousel">
                <ol class="carousel-indicators">
                    {% for image in site.data.carousel_img.images %}
                    {% if forloop.first %}
                    <li data-target="#mango-images" data-slide-to="0" class="active"></li>
                    {% else %}
                    <li data-target="#mango-images" data-slide-to="{{ forloop.index }}"></li>
                    {% endif %}
                    {% endfor %}
                </ol>
                <div class="carousel-inner" role="listbox">
                    {% for image in site.data.carousel_img.images %}
                    {% if forloop.first %}
                    <div class="carousel-item active">
                    {% else %}
                    <div class="carousel-item">
                    {% endif %}
                        <div class="view">
                            <img class="d-block w-100"
                                src='{{ image.src | relative_url }}'
                                alt="{{ image.caption }}">
                            <div class="mask rgba-black-light"></div>
                        </div>
                        <div class="carousel-caption">
                            <h2 class="h2-responsive">{{ image.caption }}</h2>
                        </div>
                    </div>
                    {% endfor %}
                </div>
                <a class="carousel-control-prev" href="#mango-images" role="button" data-slide="prev">
                    <span class="carousel-control-prev-icon" aria-hidden="true"></span>
                    <span class="sr-only">Previous</span>
                </a>
                <a class="carousel-control-next" href="#mango-images" role="button" data-slide="next">
                    <span class="carousel-control-next-icon" aria-hidden="true"></span>
                    <span class="sr-only">Next</span>
                </a>
            </div>
        </div>
    </div>
</div>