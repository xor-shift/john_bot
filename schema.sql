create table if not exists clients_telegram (
  user_id bigint not null,
  token char(35) not null,

  enabled bool not null default true,

  primary key (user_id)
);

create table if not exists clients_irc (
  server varchar(255) not null,
  port int unsigned not null,

  password varchar(255) default null,
  username varchar(255) not null,
  realname varchar(255) not null,

  enabled bool not null default true
);

create table if not exists irc_nick_choices (
  irc_id int not null,
  choice_no int not null,

  nick varchar(255) not null,

  primary key (irc_id, choice_no),
  foreign key (irc_id) references clients_irc(id)
);

create table if not exists irc_channels (
  irc_id int not null,
  channel_no int not null,

  channel varchar(255) not null,

  primary key (irc_id, channel_no),
  foreign key (irc_id) references clients_irc(id)
);

create table if not exists relay_mappings (
  from_kv text not null,
  to_kv text not null,

  primary key (from_kv, to_kv)
);

create table if not exists display_names (
  sender_kv text not null,
  display_name text not null,

  primary key (sender_kv)
);

create table if not exists user_levels (
  user_kv text not null,
  level int not null,

  primary key (user_kv)
);
