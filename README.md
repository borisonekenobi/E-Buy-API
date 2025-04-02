# E-Buy-API

## Table of Contents
- [E-Buy-API](#e-buy-api)
  - [Table of Contents](#table-of-contents)
  - [Authentication Endpoints](#authentication-endpoints)
    - [Sign Up](#sign-up)
    - [Change Password](#change-password)
    - [Sign In](#sign-in)
    - [Refresh Tokens](#refresh-tokens)
  - [Post Endpoints](#post-endpoints)
    - [Create Post](#create-post)
    - [Get Posts](#get-posts)
    - [Get Post by ID](#get-post-by-id)

## Authentication Endpoints

### Sign Up

#### Request

`POST` /api/sign-up

```json
{
  "name": "John Doe",
  "username": "johndoe",
  "password": "password"
}
```

#### Response

`201 Created`

```json
{
  "message": "User created successfully"
}
```

### Change Password

#### Request

- Requires authentication

`POST` /api/change-password

```json
{
  "old_password": "password123",
  "new_password": "newpassword456"
}
```

#### Response

`200 OK`

```json
{
  "message": "Password changed successfully"
}
```

### Sign In

#### Request

`POST` /api/sign-in

```json
{
  "username": "johndoe",
  "password": "password123"
}
```

#### Response

`200 OK`

```json
{
  "user": {
    "id": "0a2e62ee-b04d-43fd-a633-0fa0ed93446a",
    "name": "John Doe",
    "username": "johndoe",
    "type": "standard"
  },
  "access": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJkYXRhIjoie1wiaWRcIjpcIjBhMmU2MmVlLWIwNGQtNDNmZC1hNjMzLTBmYTBlZDkzNDQ2YVwiLFwibmFtZVwiOlwiSm9obiBEb2VcIixcInN0YXR1c1wiOlwiYWN0aXZlXCIsXCJ0eXBlXCI6XCJzdGFuZGFyZFwiLFwidXNlcm5hbWVcIjpcImpvaG5kb2VcIn0iLCJleHAiOjE3NDM2MzAzNzcsImlzcyI6ImF1dGgwIn0.tUzuYSsH_nw5bH9Z8T6xDrOqY9NpTDIEUI70KOuUsms",
  "refresh": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJkYXRhIjoie1wiaWRcIjpcIjBhMmU2MmVlLWIwNGQtNDNmZC1hNjMzLTBmYTBlZDkzNDQ2YVwiLFwibmFtZVwiOlwiSm9obiBEb2VcIixcInN0YXR1c1wiOlwiYWN0aXZlXCIsXCJ0eXBlXCI6XCJzdGFuZGFyZFwiLFwidXNlcm5hbWVcIjpcImpvaG5kb2VcIn0iLCJleHAiOjE3NDYyMTg3NzcsImlzcyI6ImF1dGgwIn0.bilvcZgMHani1-gfuyCnXuw-2iAJDzSm7MZlQxc0Ht0"
}
```

### Refresh Tokens

#### Request

- Requires authentication (with refresh token)

`POST` /api/refresh

#### Response

`200 OK`

```json
{
  "access": "ey",
  "refresh": "ey"
}
```

## Post Endpoints

### Create Post

#### Request

- Requires authentication

`POST` /api/posts

```json
{
  "title": "My First Post",
  "description": "This is the content of my first post.",
  "price": 19.99,
  "type": "sale"
}
```

#### Response

`201 Created`

```json
{
  "message": "Post created successfully"
}
```

### Get Posts

#### Request

`GET` /api/posts

#### Response

`200 OK`

```json
[
  {
    "id": "",
    "user_id": "",
    "title": "My First Post",
    "description": "This is the content of my first post.",
    "price": 19.99,
    "type": "sale",
    "transaction": {
      "id": "",
      "user_id": "",
      "price": 19.99
    }
  },
  {
    "id": "",
    "user_id": "",
    "title": "My Second Post",
    "description": "This is the content of my second post.",
    "price": 29.99,
    "type": "auction",
    "bids": [
      {
        "id": "",
        "user_id": "",
        "price": 31.00
      }
    ]
  }
]
```

### Get Post by ID

#### Request

`GET` /api/posts/{id}

#### Response

`200 OK`

```json
{
  "id": "",
  "user_id": "",
  "title": "My First Post",
  "description": "This is the content of my first post.",
  "price": 19.99,
  "type": "sale",
  "transaction": {
    "id": "",
    "user_id": "",
    "price": 19.99
  }
}
```
